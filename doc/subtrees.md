Some third-party software is included in this repo using git subtree. The following commands can be used to update them (changing the version tag in the command)

## binutils
```
git subtree pull --prefix .src/binutils git://sourceware.org/git/binutils-gdb.git binutils-2_39 --squash
```

## GCC
```
git subtree pull --prefix src/gcc git://gcc.gnu.org/git/gcc.git releases/gcc-12.2.0 --squash
```

There will likely be merge conflicts in generated files like `Makefile.in` and `configure`. Use the following to updadte them:

```
cd src/gcc/libsanitizer
automake
autoupdate
autoconf
```

## Musl
```
git subtree pull --prefix src/musl git://git.musl-libc.org/musl v1.2.3 --squash
```

## ACPICA
```
git subtree pull --prefix src/ACPIHAL/acpica https://github.com/acpica/acpica.git R03_31_22 --squash
```

## GNU EFI
```
git subtree pull --prefix src/gnu-efi https://git.code.sf.net/p/gnu-efi/code 3.0.15 --squash
```

## GRUB (TODO)

GRUB is currently expected to be installed on the system (as a package, does not necessarily need to be used as bootl)

```
git subtree add --prefix src/external/grub https://git.savannah.gnu.org/git/grub.git grub-2.06 --squash
```

