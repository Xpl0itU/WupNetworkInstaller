FROM wiiuenv/devkitppc:20220507

# COPY --from=wiiulegacy/dynamic_libs:0.1 /artifacts $DEVKITPRO/portlibs
# COPY --from=wiiulegacy/libutils:0.1 /artifacts $DEVKITPRO/portlibs
# RUN git clone https://github.com/devkitPro/wut.git && cd wut && make -j 4 && make install && cd ..
# COPY --from=wiiuenv/libiosuhax:latest /artifacts $DEVKITPRO
# RUN git clone https://github.com/Crementif/libiosuhax.git && cd libiosuhax && make && make install && cd ..
# RUN git clone https://github.com/Crementif/libfat.git && cd libfat && make wiiu-release -j 8 && cp include/* $DEVKITPRO/portlibs/wiiu/include && cp wiiu/lib/* $DEVKITPRO/portlibs/wiiu/lib