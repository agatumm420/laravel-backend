PHP_ARG_WITH(sqlite3, whether to enable the SQLite3 extension,
[ --with-sqlite3[=DIR]	Enables the SQLite3 extension. DIR is the prefix to SQLite3 installation directory.], no)

if test $PHP_SQLITE3 != "no"; then
	sqlite3_extra_sources=""
	PHP_SQLITE3_CFLAGS=""
	if test $PHP_SQLITE3 != "yes"; then
		AC_MSG_CHECKING([for sqlite3 files in default path])
		for i in $PHP_SQLITE3 /usr/local /usr; do
			if test -r $i/include/sqlite3.h; then
				SQLITE3_DIR=$i
				AC_MSG_RESULT(found in $i)
				break
			fi
		done

		if test -z "$SQLITE3_DIR"; then
			AC_MSG_RESULT([not found])
			AC_MSG_ERROR([Please reinstall the sqlite distribution from http://www.sqlite.org])
		fi

		AC_MSG_CHECKING([for SQLite 3.3.9+])
		PHP_CHECK_LIBRARY(sqlite3, sqlite3_prepare_v2, [
			AC_MSG_RESULT(found)
			PHP_ADD_LIBRARY_WITH_PATH(sqlite3, $SQLITE3_DIR/$PHP_LIBDIR, SQLITE3_SHARED_LIBADD)
			PHP_ADD_INCLUDE($SQLITE3_DIR/include)
		],[
			AC_MSG_RESULT([not found])
			AC_MSG_ERROR([Please install SQLite 3.3.9 first or check libsqlite3 is present])
		],[
			-L$SQLITE3_DIR/$PHP_LIBDIR -lm
		])

		PHP_CHECK_LIBRARY(sqlite3,sqlite3_key,[
			AC_DEFINE(HAVE_SQLITE3_KEY, 1, [have commercial sqlite3 with crypto support])
		])

	else
		AC_MSG_CHECKING([built in sqlite3 library])
		AC_MSG_RESULT([yes])

		sqlite3_extra_sources="libsqlite/attach.c libsqlite/auth.c libsqlite/bitvec.c libsqlite/btmutex.c libsqlite/btree.c \
		libsqlite/build.c libsqlite/callback.c libsqlite/date.c libsqlite/delete.c libsqlite/expr.c \
		libsqlite/fault.c libsqlite/func.c libsqlite/fts3.c libsqlite/fts3_hash.c libsqlite/fts3_icu.c \
		libsqlite/fts3_porter.c libsqlite/fts3_tokenizer.c libsqlite/fts3_tokenizer1.c \
		libsqlite/hash.c libsqlite/insert.c libsqlite/journal.c libsqlite/legacy.c \
		libsqlite/main.c libsqlite/malloc.c libsqlite/mutex.c libsqlite/mutex_unix.c libsqlite/mutex_w32.c libsqlite/mem1.c \
		libsqlite/os_unix.c libsqlite/os_win.c libsqlite/os.c \
		libsqlite/pager.c libsqlite/pragma.c libsqlite/prepare.c \
		libsqlite/printf.c libsqlite/random.c libsqlite/select.c \
		libsqlite/table.c libsqlite/tokenize.c libsqlite/analyze.c libsqlite/complete.c \
		libsqlite/trigger.c libsqlite/update.c libsqlite/utf.c libsqlite/util.c \
		libsqlite/vacuum.c libsqlite/vdbeapi.c libsqlite/vdbeaux.c libsqlite/vdbe.c libsqlite/vdbeblob.c \
		libsqlite/vdbemem.c libsqlite/where.c libsqlite/parse.c libsqlite/opcodes.c \
		libsqlite/alter.c libsqlite/vdbefifo.c libsqlite/vtab.c libsqlite/loadext.c"

		if test "$enable_maintainer_zts" = "yes"; then
			threadsafe_flags="-DSQLITE_THREADSAFE=1"
		else
			threadsafe_flags="-DSQLITE_THREADSAFE=0"
		fi

		if test "$ZEND_DEBUG" = "yes"; then
			debug_flags="-DSQLITE_DEBUG=1"
		fi

		other_flags="-DSQLITE_ENABLE_FTS3=1 -DSQLITE_CORE=1"

dnl		if test "$PHP_MAJOR_VERSION" -ge "6"; then
dnl			other_flags="$other_flags -DSQLITE_ENABLE_ICU=1"
dnl		fi

		PHP_SQLITE3_CFLAGS="-I@ext_srcdir@/libsqlite -I@ext_builddir@/libsqlite $other_flags $threadsafe_flags $debug_flags"
	fi

	AC_DEFINE(HAVE_SQLITE3,1,[ ])

	sqlite3_sources="sqlite3.c $sqlite3_extra_sources"

	PHP_NEW_EXTENSION(sqlite3, $sqlite3_sources, $ext_shared, ,$PHP_SQLITE3_CFLAGS)
	PHP_SUBST(SQLITE3_SHARED_LIBADD)

fi
