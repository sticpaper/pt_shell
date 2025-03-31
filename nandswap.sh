#! /vendor/bin/sh

# f2fs_io=/system_ext/bin/f2fs_io
f2fs_io=/data/amktiao/f2fs_io # debug
oplus_nandswap_stat=persist.sys.oplus.nandswap
oplus_nandswap_enable=persist.sys.oplus.nandswap.condition
oplus_nandswap_size=persist.sys.oplus.nandswap.swapsize

function run_oplus_nandswap_ocheck()
{
    if [ $(grep -o "zram[0-9]" /proc/swaps) == "zram0" ]; then
        swapoff /dev/block/zram0
        echo 1 > /sys/block/zram0/reset
    fi
}

function configure_zram_parameters()
{
    nandswap_path="/data/nandswap"
    nandswap_size=$(getprop $oplus_nandswap_size)
    zram_sysfs="/sys/block/zram0"

    if [ ! -d $nandswap_path ]; then
        mkdir /data/nandswap
    fi

    if [ -f $nandswap_path/swapfile ]; then
        rm $nandswap_path/swapfile
    fi

    if [ ! $(df -h /data | awk 'NR==2 {print $5+0}') -ge 80 ]; then
        nandswap_disk=$(losetup -f)
        touch $nandswap_path/swapfile
        $f2fs_io pinfile set $nandswap_path/swapfile
        fallocate -o 0 -l ${nandswap_size}G $nandswap_path/swapfile
        losetup "$nandswap_disk" $nandswap_path/swapfile
        echo "$nandswap_disk" > $zram_sysfs/backing_dev
    fi

    echo ${nandswap_size}G > $zram_sysfs/disksize

    mkswap /dev/block/zram0
    swapon /dev/block/zram0 -p 32765

    # set apply oplus_system property
    setprop $oplus_nandswap_enable true
}

function oplus_nandswap_config()
{
    nandswap_enable=$(getprop $oplus_nandswap_stat)

    if [ $nandswap_enable == "true" ]; then
        [ ! -f $f2fs_io ] && exit 0
        run_oplus_nandswap_ocheck
        configure_zram_parameters
    fi
}

# main-nandswap
oplus_nandswap_config
