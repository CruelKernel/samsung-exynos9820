# Cruel Kernel Tree for Samsung S10, Note10 devices

![CI](https://github.com/CruelKernel/samsung-exynos9820/workflows/CI/badge.svg)

Based on samsung sources and android common tree.
Supported devices: G970F, G973F, G975F, N970F, N971F.

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

## How to customize the kernel

It's possible to customize the kernel and build it from the browser.
First of all, create and account on GitHub. Next, **fork** this repository.
**Switch** to the "Actions" tab and activate GitHub Actions. At this step you've
got your copy of the sources and you can build it with GitHub Actions. You need
to open github actions [configuration file](.github/workflows/main.yml) and
**edit** it from the browser. For example, to alter the kernel configuration
you need to edit lines:
```YAML
    - name: Kernel Configure
      run: |
        ./build config                    \
                model=G973F               \
                name=CRUEL-V3-BTA8        \
                +magisk                   \
                +nohardening              \
                +ttl                      \
                +wireguard                \
                +cifs
...
    - name: Create boot.img
      run: ./build :mkimg model=G973F
```

First of all, you need to change G973F model to the model of your phone.
Supported models: G970F, G973F, G975F, N970F, N971F.

You can change the name of the kernel by replacing ```name=CRUEL-V3-BTA8``` with,
for example, ```name=my_own_kernel```. You can remove wireguard from the kernel
if you don't need it by changing "+" to "-" or by removing the "+wireguard" line
and "\\" on the previous line.

Available configuration presets can be found at [configs](kernel/configs/) folder.
Only the *.conf files prefixed with "cruel" are meaningful.
For example:
* magisk - integrates magisk into the kernel. This allows to have root without
  booting from recovery. Enabled by default.
* magisk+canary - integrates canary magisk into the kernel.
* ttl - adds iptables filters for altering ttl values of network packets. This
  helps to bypass tethering blocking in mobile networks.
* wireguard - adds wireguard module to the kernel.
* cifs - adds CIFS fs support.
* nohardening - removes Samsung kernel self-protection mechanisms. Potentially
  can increase the kernel performance. Enabled by default. Disable this if you
  want to make your system more secure.
* nohardening2 - removes Android kernel self-protection mechanisms. Potentially
  can increase the kernel performance. Don't use it if you don't know what you are
  doing. Almost completely disables kernel self-protection. Very insecure.
* nodebug - remove debugging information from the kernel.
* 300hz - increases kernel clock rate from 250hz to 300hz. Potentially can
  decrease response time. Disabled by default, untested.
* 1000hz - increases kernel clock rate from 250hz to 1000hz. Potentially can
  decrease response time. Disabled by default, untested.

For example, you can alter default configuration to something like:
```YAML
    - name: Kernel Configure
      run: |
        ./build config                    \
                model=G975F               \
                name=CRUEL-V3-BTA8        \
                +magisk+canary            \
                +wireguard                \
                +nohardening
...
    - name: Create boot.img
      run: ./build :mkimg model=G975F
```

After editing the configuration in the browser, save it and **commit**.
Next, you need to **switch** to the "Actions" tab. At this step you will find that
GitHub starts to build the kernel. You need to **wait** about 25-30 mins while github builds
the kernel. If the build is successfully finished, you will find your boot.img in the Artifacts
section. Download it, unzip and flash.

To keep your version of the sources in sync with main tree, please look at this tutorial:
https://stackoverflow.com/questions/20984802/how-can-i-keep-my-fork-in-sync-without-adding-a-separate-remote/21131381#21131381

## How to build the kernel locally on your PC

This instructions assumes you are using Linux. Install mkbootimg, heimdall
(if you want to flash the kernel automatically).

Next:
```sh
$ git clone https://github.com/CruelKernel/samsung-exynos9820
$ cd samsung-exynos9820
# List available branches
$ git branch -a | grep remotes | grep cruel | cut -d '/' -f 3
# Switch to the branch you need
$ git checkout cruel-v3
# Install compilers
$ git submodule update --init --recursive
# Compile
$ ./build mkimg name=CustomCruel +magisk+canary +wireguard +ttl +cifs +nohardening
# You will find your kernel in boot.img file after compilation.
$ ls -lah ./boot.img

# You can automatically flash the kernel with heimdall
# if you connect your phone to the PC and execute:
$ ./build :flash

# Or in a single command (compilation with flashing)
# ./build flash name=CustomCruel +magisk+canary +wireguard +ttl +cifs +nohardening
```

## Support

- [Telegram](https://t.me/joinchat/GsJfBBaxozXvVkSJhm0IOQ)
- [XDA Thread](https://forum.xda-developers.com/galaxy-s10/development/kernel-cruel-kernel-s10-v2-t4048707)
