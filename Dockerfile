FROM wiiuenv/devkitppc:20220507

RUN git clone https://github.com/Crementif/libiosuhax.git && cd libiosuhax && make && make install