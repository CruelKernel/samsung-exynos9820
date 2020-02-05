# Cruel Kernel Tree for Samsung S10, Note10 devices

![CI](https://github.com/CruelKernel/samsung-exynos9820/workflows/CI/badge.svg)

Based on samsung sources and android common tree.
Supported devices: G970F/N, G973F/N, G975F/N G977B/N, N970F, N975F,
N971N, N976B/N.

## Contributors

- fart1-git - for removing vendor check of DP cables in DEX mode
- NZNewbie - for adding fiops scheduler
- ExtremeGrief - for overall improvements, porting maple scheduler
- thehacker911 - overall improvements and advices
- @bamsbamx - ported boeffla\_wakelock\_blocker module
- Nico (@NicoMax2012) - ported moro sound module

## How to install

First of all, TWRP Recovery + multidisabler should be installed in all cases.
It's a preliminary step. Next, backup your existing kernel. You will be able
to restore it from TWRP Recovery in case of problems.

### How to install zip file

#### TWRP

Reboot to TWRP. Flash CruelKernel.zip. Reboot to system.

### How to install img file (raw image)

#### TWRP

Reboot to TWRP. Flash boot.img to the boot slot. Reboot to system.

#### ADB/Termux (root required)

With ADB:
```sh
$ adb shell push boot.img /sdcard/
$ adb shell su -c 'dd if=/sdcard/boot.img of=/dev/block/by-name/boot'
$ adb shell rm -f /sdcard/boot.img
$ adb reboot
```

With Termux:
```sh
# Download image on the phone and copy image to, for example, /sdcard/boot.img
$ su
$ dd if=/sdcard/boot.img of=/dev/block/by-name/boot
$ rm -f /sdcard/boot.img
$ reboot
```

#### Flashify or FKM (root required)

Just flash one of boot.img files suitable for your phone's model in the app.

#### Heimdall

Reboot to Download Mode.
```bash
$ sudo heimdall flash --BOOT boot.img
```
Reboot to system.

## Pin problem (Can't login)

The problem is not in sources. It's due to os\_patch\_level mismatch with you current
kernel (and/or twrp). CruelKernel uses common security patch date to be in sync with
the official twrp and samsung firmware. You can check the default os\_patch\_level in
cruel/build.mkbootimg.* files. However, this date can be lower than other kernels use. When
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

It's possible to customize the kernel and build it in a web browser.
First of all, you need to create an account on GitHub. Next, **fork**
this repository. **Switch** to the "Actions" tab and activate GitHub Actions.
At this step you've got your copy of the sources and you can build it with
GitHub Actions. You need to open github actions [configuration file](.github/workflows/main.yml)
and **edit** it from the browser.

First of all, you need to edit model argument (by default it's G973F) to the model
of your phone. You can select multiple models. Supported models are: G970F/N, G973F/N,
G975F/N, G977B/N, N970F, N971N, N975F, N976B/N.

Edit model:
```YAML
    strategy:
      matrix:
        model: [ "G973F" ]
```

For example, you can add two models. This will create separate
installers for models:
```YAML
    strategy:
      matrix:
        model: [ "G973F", "G975F" ]
```

If you want one installer for 2 kernels, use:
```YAML
    strategy:
      matrix:
        model: [ "G973F,G975F" ]
```

To alter the kernel configuration you need to edit lines:
```YAML
    - name: Kernel Configure
      run: |
        ./cruelbuild config                    \
                     model=${{ matrix.model }} \
                     name="Cruel-v4.0"         \
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

You can change the name of the kernel by replacing ```name="Cruel-v4.0"``` with,
for example, ```name="my_own_kernel"```. You can remove wireguard from the kernel
if you don't need it by changing "+" to "-" or by removing the "+wireguard" line
and "\\" on the previous line.

OS patch date can be changed with ```os_patch_level=2020-12``` argument,
the default current date is in cruel/build.mkbootimg.G973F file.

### Preset configurations

Available configuration presets can be found in [configs](kernel/configs/) folder.
Only the *.conf files prefixed with "cruel" are meaningful.
Presets list (+ means enabled by default, use NODEFAULTS=1 env var to drop them):
* +magisk - integrates magisk into the kernel. This allows to have root without
  booting from recovery. Enabled by default. It's possible to specify magisk version,
  e.g. +magisk=canary or +magisk=alpha or +magisk=v20.4 or +magisk=v19.4
* dtb - build dtb/dtbo images
* empty\_vbmeta - include empty vbmeta img in installer and flash it
* always\_permit - pin SELinux to always use permissive mode. Required on LOS rom.
* always\_enforce - pin SELinux to always use enforcing mode.
* +force\_dex\_wqhd - disable vendor check of DP cables in DEX mode and always use WQHD resolution.
* 25hz - decrease interrupt clock freq from 250hz to 25hz.
* 50hz - decrease interrupt clock freq from 250hz to 50hz.
* 100hz - decrease interrupt clock freq from 250hz to 100hz.
* 300hz - increase interrupt clock freq from 250hz to 300hz.
* 1000hz - increase interrupt clock freq from 250hz to 1000hz. Don't use it if you
  play games. You could benefit from this setting only if you use light/middle-weight
  apps. Look here for more info: https://source.android.com/devices/tech/debug/jank\_jitter
* fp\_boost - fingerprint boost, max freqs for faster fingerprint check.
* noatime - mount fs with noatime by default.
* simple\_lmk - use simple low memory killer instead of lmdk.
* io\_bfq - enable BFQ MQ I/O scheduler in the kernel. BFQ is multi-queue scheduler, enabling
  it requires switching SCSI subsystem to MQ mode. This means you will loose the ability
  to use cfq and other single-queue schedulers after enabling +bfq.
* io\_maple - enable MAPLE I/O scheduler in the kernel.
* io\_fiops - enable FIOPS I/O scheduler in the kernel.
* io\_sio - enable SIO I/O scheduler in the kernel.
* io\_zen - enable ZEN I/O scheduler in the kernel.
* io\_anxiety - enable Anxiety I/O scheduler in the kernel.
* io\_noop - use no-op I/O scheduler by default (it's included in kernel in all cases).
* io\_cfq - make CFQ I/O scheduler default one. CFQ is enabled by default if you are not
  enabling other schedulers. This switch is relevant only in case you enable multiple
  schedulers and want cfq to be default one, for example: +maple +fiops will make fiops
  default scheduler and give you the ability to switch to maple at runtime. Thus: +maple
  +fiops +zen +cfq will add to the kernel maple, fiops, zen and make cfq scheduler default.
* +sdfat - use sdfat for exFAT and VFAT filesystems.
* +ntfs - enable ntfs filesystem support (read only).
* +cifs - adds CIFS fs support.
* tcp\_cubic - enable CUBIC TCP congestion control.
* tcp\_westwood - enable WestWood TCP congestion control.
* tcp\_htcp - enable HTCP congestion control.
* tcp\_bbr - enable BBR congestion control.
* tcp\_bic - make BIC TCP congestion control default one. BIC is enabled by default
  if you are not enabling other engines. This options work as +cfq but for TCP
  congestion control modules.
* sched_... - enable various (+performance, conservative, ondemand, +powersave,
  userspace) CPU schedulers in the kernel.
* ttl - adds iptables filters for altering ttl values of network packets. This
  helps to bypass tethering blocking in mobile networks.
* mass\_storage - enable usb mass storage drivers for drivedroid.
* +wireguard - adds wireguard module to the kernel.
* +morosound - enable moro sound control module.
* +boeffla\_wl\_blocker - enable boeffla wakelock blocker module.
* +nohardening - removes Samsung kernel self-protection mechanisms. Potentially
  can increase the kernel performance. Enabled by default. Disable this if you
  want to make your system more secure.
* nohardening2 - removes Android kernel self-protection mechanisms. Potentially
  can increase the kernel performance. Don't use it if you don't know what you are
  doing. Almost completely disables kernel self-protection. Very insecure. (fake_config
  to shut-up android warning)
* size - invoke compiler with size optimization flag (-Os).
* performance - invoke compiler with aggressive optimizations (-O3).
* +nodebug - remove debugging information from the kernel.
* noksm - disable Kernel Samepage Merging (KSM).
* nomodules - disable loadable modules support (fake\_config to shut-up android warning).
* noaudit - disable kernel auditing subsystem (fake\_config to shut-up android warning).
* noswap - disable swapping (fake\_config to shut-up android warning).
* nozram - disable nozram.
* usb\_serial - enable usb serial console support for nodemcu/arduino devices.
* fake\_config - Use defconfig for /proc/config.gz Some of the config presets, for
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
        ./cruelbuild config                    \
                     os_patch_level=2020-12    \
                     model=${{ matrix.model }} \
                     name="OwnKernel"          \
                     toolchain=proton          \
                     +magisk=canary            \
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

### Toolchain

It's possible to select a toolchain. For example, you can switch to default toolchain by adding
"TOOLCHAIN: default" line in the main.yml config file.

```YAML
env:
  TOOLCHAIN: default
```

Available toolchains:
 - default - standard toolchain from samsung's kernel archives for S10/Note10 models (clang6/gcc4.9)
 - cruel - stable gcc 10.3.0 with LTO+PGO optimizations and reverted default inlining params to 9.3 version (https://github.com/CruelKernel/aarch64-cruel-elf)
 - samsung - samsung's toolchain from S20 sources archive (clang8/gcc-4.9)
 - google - official toolchain from google. Clang 12.0.4 from r23 and GCC 4.9 from r21
 - proton - bleeding-edge clang 13 (https://github.com/kdrag0n/proton-clang)
 - arter97 - stable gcc 10.2.0 (https://github.com/arter97/arm64-gcc)
 - arm - arm's gcc 9.2-2019.12 (https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-a/downloads)
 - system-gcc - gcc cross compiler installed in your system
 - system-clang - clang installed in your system

## How to build the kernel locally on your PC

This instructions assumes you are using Linux. Install heimdall if you want to flash the
kernel automatically.

Next:
```sh
# Install prerequisites
# If you use ubuntu or ubuntu based distro then you need to install these tools:
$ sudo apt-get install build-essential libncurses-dev libtinfo5 bc bison flex libssl-dev libelf-dev heimdall-flash android-tools-adb android-tools-fastboot curl p7zip-full
# If you use Fedora:
$ sudo dnf group install "Development Tools"
$ sudo dnf install ncurses-devel ncurses-compat-libs bc bison flex elfutils-libelf-devel openssl-devel heimdall android-tools curl p7zip
# If you use Arch/Manjaro (from ..::M::..):
$ sudo pacman -Sy base-devel ncurses bc bison flex openssl libelf heimdall android-tools curl p7zip --needed
$ sudo link /lib/libtinfo.so.6 /lib/libtinfo.so.5

# Install avbtool
$ wget -q https://android.googlesource.com/platform/external/avb/+archive/refs/heads/master.tar.gz -O - | tar xzf - avbtool.py
$ chmod +x avbtool.py
$ sudo mv avbtool.py /usr/local/bin/avbtool

# Install mkbootimg
$ wget -q https://android.googlesource.com/platform/system/tools/mkbootimg/+archive/refs/heads/master.tar.gz -O - | tar xzf - mkbootimg.py gki
$ chmod +x mkbootimg.py
$ sudo mv mkbootimg.py /usr/local/bin/mkbootimg
$ sudo mv gki $(python -c 'import site; print(site.getsitepackages()[0])')

# Install mkdtboimg
$ wget -q https://android.googlesource.com/platform/system/libufdt/+archive/refs/heads/master.tar.gz -O - | tar --strip-components 2 -xzf - utils/src/mkdtboimg.py
$ chmod +x mkdtboimg.py
$ sudo mv mkdtboimg.py /usr/local/bin/mkdtboimg

# Get the sources
$ git clone https://github.com/CruelKernel/samsung-exynos9820
$ cd samsung-exynos9820

# List available branches
$ git branch -a | grep remotes | grep cruel | cut -d '/' -f 3
# Switch to the branch you need
$ git checkout cruel-v4.0

# Install compilers
$ git submodule update --init --depth 1 -j $(nproc)
# execute these 4 commands if you want to use non-default toolchains
# cd toolchain
# git remote set-branches origin '*'
# git fetch -v --depth 1
# cd ../

# Compile kernel for G970F, G973F, G975F phones.
# Use model=all if you want to build the kernel for all available phones.
$ ./cruelbuild mkimg name="CustomCruel" model=G970F,G973F,G975F toolchain=proton +magisk=canary +wireguard +ttl +cifs +nohardening
# You will find your kernel in boot.img file after compilation.
$ ls -lah ./boot.img

# You can automatically flash the kernel with adb/heimdall
# if you connect your phone to the PC and execute:
$ FLASH=y ./cruelbuild mkimg ...

# Or in a single command (compilation with flashing)
# FLASH=y ./cruelbuild mkimg name="CustomCruel" model=G973F toolchain=proton +magisk=canary +wireguard +ttl +cifs +nohardening
```

## Support

- [Telegram](https://t.me/joinchat/GsJfBBaxozXvVkSJhm0IOQ)
- [XDA Thread](https://forum.xda-developers.com/galaxy-s10/samsung-galaxy-s10--s10--s10-5g-cross-device-development-exynos/kernel-cruel-kernel-s10-note10-v3-t4063495)

