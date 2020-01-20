import os
import sys
import subprocess
import re
from threading import Timer

global_front = """
#include "debug-snapshot-log.h"

struct dbg_snapshot_log *p;

void remove_non_ascii(char * s, int len) {
    for (int i = 0; i < len; i++) {
        if(!s[i]) break;
        if(!(32 <= s[i] && s[i] < 127) || s[i] == '\\'') s[i] = ' ';
    }
    s[len-1] = 0;
}

int main(int argc, char *argv[])
{
    FILE *f;
    int ch;
    long fsize;
    char *string;
    int i0, i1, i2;

    f = fopen(argv[1], "rb");
    if (f == NULL) {
        fputs("file read error!", stderr);
        exit(1);
    }
    fseek(f, 0, SEEK_END);
    fsize = ftell(f);
    fseek(f, 0, SEEK_SET);  //same as rewind(f);

    string = malloc(fsize + 1);
    fread(string, fsize, 1, f);
    fclose(f);
    p = (struct dbg_snapshot_log *)string;

    printf("log = []\\n");
"""

global_back = """
    return 0;
}
"""


### Below classes substitute global variable.
class Singleton(type):
    _instances = {}
    def __call__(cls, *args, **kwargs):
        if cls not in cls._instances:
            cls._instances[cls] = super(Singleton, cls).__call__(*args, **kwargs)
        return cls._instances[cls]

def execget(cmd, to=100, printerr=False):
    def kill(process):
        process.kill();
    stdout = ""
    stderr = ""
    p = subprocess.Popen(cmd, shell="True", stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=os.environ);
    my_timer = Timer(to, kill, [p]);
    try:
        my_timer.start()
        stdout, stderr = p.communicate()
    finally:
        my_timer.cancel()
    return stdout;

def expect(current, expect, short_msg):
    if current not in expect:
        msg = """
        Error : %s
        Current output: %s
        Expect output : %s
        """ % (short_msg, current, expect)
        raise Exception(msg)

class TypeTable:
    __metaclass__ = Singleton
    def __init__(self):
        if "data" in self.__dict__:
            return
        self.data = {}
    def get(self, key):
        return self.data.get(key, None)
    def set(self, key, value):
        self.data[key] = value
    def clear(self):
        self.data = {}
    def __repr__(self):
        ret = ""
        for k, v in self.data.items():
            ret += "%s\n" % str(v)
        return ret

class FieldTable:
    __metaclass__ = Singleton
    def __init__(self):
        if "d" in self.__dict__:
            return
        self.d = []
    def find_typedecl_all(self):
        for x in self.d:
            x.find_typedecl()
### Primitive Types -----------------------------

class Decl:
    @staticmethod
    def find_depth(line):
        s = 0
        while not line[s].isalpha():
            s += 1
            if s < len(line):
                continue
            s = -1;
            break
        return s

class TypeDecl(Decl):
    def __init__(self, name, abv):
        self.name = name
        self.abv = abv
        typetab = TypeTable()
        typetab.set(self.name, self)

    def __repr__(self):
        return self.name

    @staticmethod
    def add_primative():
        TypeDecl("short", "%hd")
        TypeDecl("unsigned short", "%hu")
        TypeDecl("signed short", "%hd")

        TypeDecl("int", "%d")
        TypeDecl("unsigned int", "%u")
        TypeDecl("signed int", "%d")

        TypeDecl("unsigned char", "%u")
        TypeDecl("signed char", "%d")

        TypeDecl("long", "%ld")
        TypeDecl("unsigned long", "%lu")
        TypeDecl("signed long", "%lu")

        TypeDecl("long long", "%lld")
        TypeDecl("unsigned long long", "%llu")
        TypeDecl("signed long long", "%lld")

        TypeDecl("double", "%lf")
        TypeDecl("float", "%f")

        TypeDecl("pointer", "'%p'")
        TypeDecl("char pointer", "'%p'")

        TypeDecl("char", "'%s'")

        TypeDecl("unknown", "")

### RecordDecl Types -----------------------------

class RecordDecl(TypeDecl):
    def __init__(self, line):
        self.depth = Decl.find_depth(line)
        line = line[self.depth:]
        arr = line.strip().split(" ")
        self.id = arr[1]
        self.name = arr[-2]
        self.field = []
        typetab = TypeTable()
        typetab.set(self.name, self)

    def __repr__(self):
        ret = "%s %s\n" % (self.id, self.name)
        for x in self.field:
            ret += "  %s\n" % str(x)
        return ret

### FieldDecl Types -----------------------------

"""
  |-RecordDecl 0x5c53f80 parent 0x5aad4f8 <line:353:2, line:358:2> line:353:9 struct __printk_log definition
  | |-FieldDecl 0x5c54058 <line:354:3, col:22> col:22 time 'unsigned long long'
  | |-FieldDecl 0x5c540b8 <line:355:3, col:7> col:7 cpu 'int'
  | |-FieldDecl 0x5c54178 <line:356:3, col:33> col:8 log 'char [128]'
  | `-FieldDecl 0x5c54220 <line:357:3, col:37> col:9 caller 'void *[4]'
"""

class FieldDecl(Decl):
    def __init__(self, line, record):
        self.record = record
        self.depth = Decl.find_depth(line)
        line = line[self.depth:]
        arr = line.strip().split(" ", 6)
        self.id = arr[1]
        self.name = arr[-2]
        if "':'" in arr[-1]:
            self.type = arr[-1].split("':'")[-1].strip().strip("'")
        else:
            self.type = arr[-1].strip().strip("'")
        array = re.findall(r"\[(?P<index>\d+)\]", self.type)
        self.array = [int(i) for i in array]
        for n in self.array:
            self.type = self.type.replace("[%d]" % n ,"").strip()
        fieldtab = FieldTable()
        fieldtab.d.append(self)

    def find_typedecl(self):
        typetab = TypeTable()
        self.type = self.type.replace("struct ", "")
        self.type = self.type.replace("union ", "")
        if "*" in self.type and "char" in self.type:
            self.type = "char pointer"
        if "*" in self.type:
            self.type = "pointer"
        if typetab.get(self.type):
            self.type = typetab.get(self.type)
        else:
            self.type = typetab.get("unknown")

    def __repr__(self):
        ret = "%s %s " % (self.id, self.name)
        array = ""
        for n in self.array:
            array += "[%d]" % n
        ret += "'%s %s'" % (str(self.type), array)
        return ret

    def array_printer(self, array):
        front_array = "["
        for i in range(self.array[0]):
            front_array += "%s, " % self.type.abv
        front_array = front_array[:-1] + "]"

        mid = ""
        for i in range(self.array[0]):
            if self.type.name == "pointer":
                mid += "p->%s.%s[%d] ? p->%s.%s[%d] :(void**)-1,\n" %(array, self.name, i, array, self.name, i)
            else:
                mid += "p->%s.%s[%d],\n" %(array, self.name, i)

        front = "'%s' :%s, " % (self.name, front_array)
        return front, mid

    def printer(self, printf_obj=None):
        ## Check printer is called by root.
        if not printf_obj:
            pf = Printf(self.array, self.name)
        else:
            pf = printf_obj

        ## Check this type is primitvie
        if hasattr(self.type, "abv"):
            pf.add_field(self, pf.ref_name)
        else:
            ## structs are layered like Binder, self.name is cascaded to ref_name
            if printf_obj:
                temp = pf.ref_name
                pf.ref_name = "%s.%s" % (pf.ref_name, self.name)
            for f in self.type.field:
                f.printer(pf)
            if printf_obj:
                pf.ref_name = temp

        ## If printer is succeed in root, save all fields as string
        if not printf_obj:
            return pf.save_all()
        return None

### Printf class --------------------------------

class Printf:
    def __init__(self, bounds, name):
        self.bounds = bounds
        self.name = name
        self.ref_name = name

        ## Arrays for storing fields
        self.fields = []
        self.pointer_fields = []
        self.string_fields = []
        self.array_fields = []
        self.pointer_array_fields = []

        ## Result strings will be concatenated right before returing.
        self.for_statement = ""
        self.for_statement_back = ""
        self.for_body = ""
        self.printf_front = ""
        self.printf_format = ""
        self.printf_mid = ""
        self.printf_args = ""
        self.printf_back = ""

        ## Makeup default string for 'for statement' & 'printf'
        self.make_for_statement()
        self.prepare_printf()

    ## Field is pushed from outside by this method
    def add_field(self, x, ref_name):
        x.ref_name = ref_name
        if x.name == "time" :
            pass
        elif x.type.name == "char":
            self.string_fields.append(x)
        elif len(x.array) > 0  and x.type.name != "pointer" :
            self.array_fields.append(x)
        elif len(x.array) > 0  and x.type.name == "pointer" :
            self.pointer_array_fields.append(x)
        elif x.type.name == "pointer":
            self.pointer_fields.append(x)
        else:
            self.fields.append(x)

    def make_for_statement(self):
        for index, n in enumerate(self.bounds):
            self.for_statement += "for (i%d = 0; i%d < %d; i%d++) {\n" % (index, index, n, index)
            self.for_statement_back += "}\n"
            self.ref_name += "[i%d]" % index
        self.for_body += "if (p->%s.time == 0) break;\n" % (self.ref_name)

    def prepare_printf(self):
        self.printf_front += "printf(\"log.append({ 'type' : '%s', " % self.name
        self.printf_mid += "})\\n\",\n"
        self.printf_back += ");\n"

        self.printf_format += "'time' : %lld.%09lld, "
        self.printf_args += "p->%s.time/1000000000LL," % self.ref_name
        self.printf_args += "p->%s.time%%1000000000LL,\n" % self.ref_name
        if len(self.bounds) > 1 and self.bounds[0] == 8:
            self.printf_format += "'cpu' : %d, "
            self.printf_args += "i0,\n"

    ## Iterate all fields, and Convert it to C-code
    def process_fields(self):
        for x in self.fields:
            self.print_normal(x)
        for x in self.string_fields:
            self.print_string(x)
        for x in self.array_fields:
            self.print_array(x)
        for x in self.pointer_array_fields:
            self.print_pointer_array(x)
        for x in self.pointer_fields:
            self.print_pointer(x)

    def print_normal(self, x):
        self.printf_format += "'%s' : %s, " % (x.name, x.type.abv)
        self.printf_args += "p->%s.%s,\n" %(x.ref_name, x.name)

    def print_pointer(self, x):
        self.printf_format += "'%s' : %s, " % (x.name, x.type.abv)
        self.printf_args += "p->%s.%s ? p->%s.%s :(void**)-1,\n" %(x.ref_name, x.name, x.ref_name, x.name)

    def print_string(self, x):
        self.printf_format += "'%s' : %s, " % (x.name, x.type.abv)
        self.printf_args += "p->%s.%s,\n" %(x.ref_name, x.name)
        self.for_body += ("remove_non_ascii(p->%s.%s, %d);\n" %(x.ref_name, x.name, x.array[0]))

    def print_array(self, x):
        front_array = "["
        for i in range(x.array[0]):
            front_array += "%s, " % x.type.abv
        front_array = front_array[:-1] + "]"
        for i in range(x.array[0]):
            self.printf_args += "p->%s.%s[%d],\n" %(x.ref_name, x.name, i)
        self.printf_format += "'%s' :%s, " % (x.name, front_array)

    def print_pointer_array(self, x):
        front_array = "["
        for i in range(x.array[0]):
            front_array += "%s, " % x.type.abv
        front_array = front_array[:-1] + "]"
        for i in range(x.array[0]):
            self.printf_args += "p->%s.%s[%d] ? p->%s.%s[%d] :(void**)-1,\n" %(x.ref_name, x.name, i, x.ref_name, x.name, i)
        self.printf_format += "'%s' :%s, " % (x.name, front_array)

    def save_all(self):
        self.process_fields()
        ret = ""
        ret += self.for_statement
        ret += self.for_body
        ret += self.printf_front
        ret += self.printf_format
        ret += self.printf_mid
        ret += self.printf_args[:-2]
        ret += self.printf_back
        ret += self.for_statement_back
        return ret

### ASTdump Class -----------------------------

class ASTdump:
    def __init__(self):
        if "clang" in os.environ["CC"]:
            self.clang = os.environ["CC"]
        elif "clang" in execget("which clang"):
            self.clang = "clang"

    def run_inner(self, i, record):
        depth = record.depth
        ret = []
        for line in self.dump[i+1:]:
            inner_depth = Decl.find_depth(line)
            if depth >= inner_depth:
                break
            if depth + 2 != inner_depth:
                continue
            if not "FieldDecl" in line:
                continue
            ret.append(FieldDecl(line, record))
        return ret

    def prepare(self):
        self.dump = execget("%s -Iinclude -Xclang -ast-dump -fsyntax-only -DDSS_ANALYZER lib/debug-snapshot-log.h" % self.clang)
        self.dump = self.dump.strip().split("\n")
        for i, line in enumerate(self.dump):
            if not "RecordDecl" in line:
                continue
            record = RecordDecl(line)
            record.field = self.run_inner(i, record)
        FieldTable().find_typedecl_all()

    def run(self):
        main_root = TypeTable().get("dbg_snapshot_log")
        ret = ""
        for r in main_root.field:
            ret += r.printer()
        return global_front + ret + global_back

if __name__ == "__main__":
    TypeDecl.add_primative()
    astdump = ASTdump()
    astdump.prepare()
    ret = astdump.run()
    print(ret)

