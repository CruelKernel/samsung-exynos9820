# Cruel Kernel Tree for Samsung S10, Note10 devices

![CI](https://github.com/CruelKernel/samsung-exynos9820/workflows/CI/badge.svg)

Based on samsung sources and android common tree.
Supported devices: G970F, G970N, G973F, G975F, G977B, N970F, N975F,
N971N, N976B, N976N.

## Contributors

- NZNewbie - for adding fiops scheduler
- ExtremeGrief - for overall improvements, porting maple scheduler
- thehacker911 - overall improvements and advices
- @bamsbamx - ported boeffla_wakelock_blocker module
- Nico (@NicoMax2012) - ported moro sound module

## How to install

First of all, TWRP Recovery + multidisabler should be installed in all cases.
It's a preliminary step. Next, backup your existing kernel. You will be able
to restore it from TWRP Recovery in case of problems.

### TWRP

Reboot to TWRP Recovery. Flash boot.img in boot slot.

### Heimdall

Reboot to Download Mode.
```bash
$ sudo heimdall flash --BOOT boot.img
```

### Franco Kernel Manager

Flash boot.img with FK Manager.

## Pin problem (Can't login)

The problem is not in sources. It's due to os_patch_level mismatch with you current
kernel (and/or twrp). CruelKernel uses common security patch date to be in sync with
the official twrp and samsung firmware. You can check the default os_patch_level in
build.mkbootimg.* files. However, this date can be lower than other kernels use. When
you flash a kernel with an earlier patch date on top of the previous one with a higher
date, android activates rollback protection mechanism and you face the pin problem. It's
impossible to use a "universal" os_patch_level because different users use different
custom kernels and different firmwares. CruelKernel uses the common date by default
in order to suite most of users.

How can you solve the problem? 6 different ways:
- You can restore your previous kernel and unlock problem will gone
- You can flash [backtothefuture-2099-12.zip](https://github.com/CruelKernel/backtothefuture/releases/download/v1.0/backtothefuture-2099-12.zip)
  in TWRP to set the os_patch_level date for your boot and recovery partitions to 2099-12.
  You can use other than 2099-12 date in the zip filename. You need to set it to the same
  or greater date as your previous kernel. Nemesis and Los (from ivanmeller) kernels use 2099-12.
  Max possible date is: 2127-12. It will be used if there will be no date in the zip filename.
- You can check the os_patch_level date of your previous kernel here
  https://cruelkernel.org/tools/bootimg/ and patch cruel kernel image to the same date.
  If your previous kernel is nemesis, patch cruel to 2099-12 date.
- You can reboot to TWRP, navigate to data/system and delete 3 files those names starts
  with 'lock'. Reboot. Login, set a new pin. To fix samsung account login, reinstall the app
- You can rebuild cruel kernel with os_patch_level that suites you. To do it, you need to
  add the line os_patch_level="\<your date\>" to the main.yml cruel configuration.
  See the next section if you want to rebuild the kernel.
- You can do the full wipe during cruel kernel flashing

## How to customize the kernel

It's possible to customize the kernel and build it from a web browser.
First of all, you need to create an account on GitHub. Next, **fork**
this repository. **Switch** to the "Actions" tab and activate GitHub Actions.
At this step you've got your copy of the sources and you can build it with
GitHub Actions. You need to open github actions [configuration file](.github/workflows/main.yml)
and **edit** it from the browser.

First of all, you need to edit model argument (by default it's G973F) to the model
of your phone. You can multiple models. Supported models are: G970F, G970N, G973F, G975F,
G977B, N970F, N971N, N975F, N976B, N976N.

Edit model:
```YAML
    strategy:
      matrix:
        model: [ "G973F" ]
```

For example, you can add two models:
```YAML
    strategy:
      matrix:
        model: [ "G973F", "G975F" ]
```


To alter the kernel configuration you need to edit lines:
```YAML
    - name: Kernel Configure
      run: |
        ./build config                    \
                model=${{ matrix.model }} \
                name="CRUEL-V3.4"         \
                +magisk                   \
                +nohardening              \
                +ttl                      \
                +wireguard                \
                +cifs                     \
                +sdfat                    \
                +ntfs                     \
                +morosound                \
                +boeffla_wl_blocker
```

You can change the name of the kernel by replacing ```name="CRUEL-V3"``` with,
for example, ```name="my_own_kernel"```. You can remove wireguard from the kernel
if you don't need it by changing "+" to "-" or by removing the "+wireguard" line
and "\\" on the previous line. OS patch date can be changed with
```os_patch_level=2020-02``` argument, the default current date is in
build.mkbootimg.G973F file.

Available configuration presets can be found at [configs](kernel/configs/) folder.
Only the *.conf files prefixed with "cruel" are meaningful.
For example:
* +magisk - integrates magisk into the kernel. This allows to have root without
  booting from recovery. Enabled by default.
* magisk+canary - integrates canary magisk into the kernel.
* always_permit - pin SELinux to always use permissive mode. Required on LOS rom.
* always_enforce - pin SELinux to always use enforcing mode.
* 300hz - increase interrupt clock freq from 250hz to 300hz.
* 1000hz - increase interrupt clock freq from 250hz to 1000hz. Don't use it if you
  play games. You could benefit from this setting only if you use light/middle-weight
  apps. Look here for more info: https://source.android.com/devices/tech/debug/jank_jitter
* noatime - mount fs with noatime by default.
* io_bfq - enable BFQ MQ I/O scheduler in the kernel. BFQ is multi-queue scheduler, enabling
  it requires switching SCSI subsystem to MQ mode. This means you will loose the ability
  to use cfq and other single-queue schedulers after enabling +bfq.
* io_maple - enable MAPLE I/O scheduler in the kernel.
* io_fiops - enable FIOPS I/O scheduler in the kernel.
* io_sio - enable SIO I/O scheduler in the kernel.
* io_zen - enable ZEN I/O scheduler in the kernel.
* io_anxiety - enable Anxiety I/O scheduler in the kernel.
* io_noop - use no-op I/O scheduler by default (it's included in kernel in all cases).
* io_cfq - make CFQ I/O scheduler default one. CFQ is enabled by default if you are not
  enabling other schedulers. This switch is relevant only in case you enable multiple
  schedulers and want cfq to be default one, for example: +maple +fiops will make fiops
  default scheduler and give you the ability to switch to maple at runtime. Thus: +maple
  +fiops +zen +cfq will add to the kernel maple, fiops, zen and make cfq scheduler default.
* sdfat - use sdfat for exFAT and VFAT filesystems.
* ntfs - enable ntfs filesystem support (read only).
* cifs - adds CIFS fs support.
* tcp_cubic - enable CUBIC TCP congestion control.
* tcp_westwood - enable WestWood TCP congestion control.
* tcp_htcp - enable HTCP congestion control.
* tcp_bbr - enable BBR congestion control.
* tcp_bic - make BIC TCP congestion control default one. BIC is enabled by default
  if you are not enabling other engines. This options work as +cfq but for TCP
  congestion control modules.
* sched_... - enable various (performance, conservative, ondemand, powersave,
  userspace) CPU schedulers in the kernel.
* ttl - adds iptables filters for altering ttl values of network packets. This
  helps to bypass tethering blocking in mobile networks.
* mass_storage - enable usb mass storage drivers for drivedroid.
* wireguard - adds wireguard module to the kernel.
* morosound - enable moro sound control module.
* boeffla_wl_blocker - enable boeffla wakelock blocker module.
* +nohardening - removes Samsung kernel self-protection mechanisms. Potentially
  can increase the kernel performance. Enabled by default. Disable this if you
  want to make your system more secure.
* nohardening2 - removes Android kernel self-protection mechanisms. Potentially
  can increase the kernel performance. Don't use it if you don't know what you are
  doing. Almost completely disables kernel self-protection. Very insecure. (fake_config
  to shut-up android warning)
* size - optimize kernel for size.
* nodebug - remove debugging information from the kernel.
* noksm - disable Kernel Samepage Merging (KSM).
* nomodules - disable loadable modules support (fake_config to shut-up android warning).
* noaudit - disable kernel auditing subsystem (fake_config to shut-up android warning).
* noswap - disable swapping (fake_config to shut-up android warning).
* nozram - disable nozram.
* fake_config - Use defconfig for /proc/config.gz Some of the config presets, for
  example nomodules, noaudit are safe but Android system checks kernel configuration
  for these options to be enabled and issues the warning "There's an internal problem
  with your device. Contact your manufacturer for details." in case they are not. This
  config preset forces default configuration to be in /proc/config.gz This trick allows
  to pass Android system check and shut up the warning. However, the kernel will use
  other configuration during build.

For example, you can alter default configuration to something like:
```YAML
    - name: Kernel Configure
      run: |
        ./build config                    \
                os_patch_level=2020-12    \
                model=${{ matrix.model }} \
                name="OwnKernel"          \
                +magisk+canary            \
                +wireguard                \
                +nohardening              \
                +1000hz
```

After editing the configuration in the browser, save it and **commit**.
Next, you need to **switch** to the "Actions" tab. At this step you will find that
GitHub starts to build the kernel. You need to **wait** about 25-30 mins while github builds
the kernel. If the build is successfully finished, you will find your boot.img in the Artifacts
section. Download it, unzip and flash.

To keep your version of the sources in sync with main tree, please look at one of these tutorials:
- [How can I keep my fork in sync without adding a separate remote?](https://stackoverflow.com/a/21131381)
- [How do I update a GitHub forked repository?](https://stackoverflow.com/a/23853061)

## How to build the kernel locally on your PC

This instructions assumes you are using Linux. Install heimdall if you want to flash the
kernel automatically.

Next:
```sh
# Install prerequisites
# If you use ubuntu or ubuntu based distro then you need to install these tools:
$ sudo apt-get install build-essential libncurses-dev libtinfo5 bc bison flex libssl-dev libelf-dev
# If you use Fedora:
$ sudo dnf group install "Development Tools"
$ sudo dnf install ncurses-devel ncurses-compat-libs bc bison flex elfutils-libelf-devel openssl-devel

# Install mkbootimg
$ wget -q https://android.googlesource.com/platform/system/tools/mkbootimg/+archive/refs/heads/master.tar.gz -O - | tar xzf - mkbootimg.py
$ chmod +x mkbootimg.py
$ sudo mv mkbootimg.py /usr/local/bin/mkbootimg

# Get the sources
$ git clone https://github.com/CruelKernel/samsung-exynos9820
$ cd samsung-exynos9820
# List available branches
$ git branch -a | grep remotes | grep cruel | cut -d '/' -f 3
# Switch to the branch you need
$ git checkout cruel-v3
# Install compilers
$ git submodule update --init --recursive
# Compile
$ ./build mkimg name="CustomCruel" model=G973F +magisk+canary +wireguard +ttl +cifs +nohardening
# You will find your kernel in boot.img file after compilation.
$ ls -lah ./boot.img

# You can automatically flash the kernel with heimdall
# if you connect your phone to the PC and execute:
$ ./build :flash

# Or in a single command (compilation with flashing)
# ./build flash name="CustomCruel" model=G973F +magisk+canary +wireguard +ttl +cifs +nohardening
```

## Support

- [Telegram](https://t.me/joinchat/GsJfBBaxozXvVkSJhm0IOQ)
- [XDA Thread](https://forum.xda-developers.com/galaxy-s10/samsung-galaxy-s10--s10--s10-5g-cross-device-development-exynos/kernel-cruel-kernel-s10-note10-v3-t4063495)

