#!/bin/sh

IFACE=10.0.1.16
REGISTRAR="--register 10.0.1.16"

./obj_linux_x86_64/opalmcu \
   -tttt --output $HOME/temp/mcu.log \
   --h323 tcp\$${IFACE}:21720 \
   --sip  udp\$${IFACE}:25060 \
   --sip  tcp\$${IFACE}:25060 \
   --sip  tls\$${IFACE}:25061 \
   $REGISTRAR \
   --user conf \
   --name conf \
   --factory newconf \
   --attendant test.vxml \
   --size 'cif@15' \

