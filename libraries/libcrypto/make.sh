#PKGURL=https://www.openssl.org/source/openssl-1.1.1d.tar.gz
#PKGHASH=056057782325134b76d1931c48f2c7e6595d7ef4
PKGURL=https://www.openssl.org/source/openssl-1.1.1g.tar.gz
PKGHASH=b213a293f2127ec3e323fb3cfc0c9807664fd997

package_download $PKGURL $PKGHASH

package_patch
rmifexists $SYS_PREFIX/include/openssl

EXTRACONF=
case $SYS_PLATFORM in
win32*)
  EXTRACONF="no-engine mingw"
;;
linux486*)
  EXTRACONF=linux-generic32
;;
linux*)
  cpu=
  if [ $SYS_PLATFORM = $SYS_HOSTPLATFORM ]; then
    cpu=`$SYS_ROOT/config.guess | cut -f 1 -d "-"`
  fi
  if [ "X$cpu" = "Xx86_64" ]; then
    EXTRACONF=linux-x86_64
  else
    EXTRACONF=linux-generic32
  fi
;;
macosx)
  if [ "$KERNEL_BITS" = "32" ]; then
      EXTRACONF="darwin-i386-cc"
  else
      EXTRACONF="darwin64-x86_64-cc"
  fi
;;
android*)
    EXTRACONF="android-arm -D__ANDROID_API__=$SYS_ANDROIDAPI"
    XX_ANDROID_NDK_HOME=`find ${SYS_PREFIX}/*-${SYS_ANDROIDAPI}-* -name AndroidVersion.txt 2> /dev/null`
    if [ "X$XX_ANDROID_NDK_HOME" = "X" ]; then
      XX_ANDROID_NDK_HOME=`find $ANDROIDNDK/ -name AndroidVersion.txt 2> /dev/null`
      /bin/cp "$XX_ANDROID_NDK_HOME" $SYS_PREFIX/android-ndk-*-toolchain
      XX_ANDROID_NDK_HOME=`find $SYS_PREFIX -name AndroidVersion.txt 2> /dev/null`
    fi
    export ANDROID_NDK_HOME=`dirname $XX_ANDROID_NDK_HOME`
    PATH=`ls -d $SYS_PREFIX/android-ndk-*-toolchain/bin`:$PATH
;;
*)
  EXTRACONF=BSD-generic32
;;
esac

case $SYS_PLATFORM in
    # Openssl 1.1.x appears take great care to defeat the use of
    # `package_configure` from lambdanative when cross compiling for
    # Android.
    android)
	./Configure --openssldir="$SYS_PREFIX" no-shared no-zlib no-asm no-dso no-sse2 $EXTRACONF || exit 1 # CC=$SYS_CC
    ;;
    *)
	cp Configure configure   # openssl ships a Configure script
	NOQUIET=yes package_configure --openssldir="$SYS_PREFIX" no-shared no-zlib no-asm no-dso no-sse2 $EXTRACONF # CC=$SYS_CC
;;
esac
package_make build_libs

cp *.a $SYS_PREFIX/lib
cp -L -R include/openssl $SYS_PREFIX/include

package_cleanup

#eof
