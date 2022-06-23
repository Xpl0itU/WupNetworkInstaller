FROM devkitpro/devkitppc

# COPY --from=wiiulegacy/dynamic_libs:0.1 /artifacts $DEVKITPRO/portlibs
COPY --from=wiiulegacy/libutils:0.1 /artifacts $DEVKITPRO/portlibs
COPY --from=wiiulegacy/libfat:1.1.3a /artifacts $DEVKITPRO/portlibs
RUN dkp-pacman -S --needed --noconfirm wiiu-dev
RUN dkp-pacman -S --needed --noconfirm ppc-portlibs wiiu-portlibs