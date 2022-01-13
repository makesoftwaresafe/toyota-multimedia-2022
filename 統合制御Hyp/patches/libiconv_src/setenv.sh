if [ -r $QNX_HOST/usr/bin/pwd.exe ]; then
        CPWD=`$QNX_HOST/usr/bin/pwd.exe`
else
        CPWD=`pwd`
fi

QCONF_OVERRIDE=$CPWD/qconf-override.mk
export QCONF_OVERRIDE
echo "INSTALL_ROOT_nto := $CPWD/install" > $QCONF_OVERRIDE
echo "USE_INSTALL_ROOT = 1" >> $QCONF_OVERRIDE
