#!/bin/bash

function slowcat() {
  while read; do sleep .5; echo -n "$REPLY"; done;
}

sleep 2

case $1 in
  ProtocolError)
    printf "\$qAttached#8f\n-\n" | slowcat | nc localhost 1337
    ;;
  WrongBegin)
    printf "\$qAttached#8f\nqSupported#37\n" | slowcat | nc localhost 1337
    ;;
  WrongChecksum)
    printf "\$qAttached#8f\n\$qSupported#36\n" | slowcat | nc localhost 1337
    ;;
  MessageTooLong)
    long_string=$(head -c 4097 < /dev/zero | tr '\0' '\141')
    printf "\$qAttached#8f\n\$${long_string}#97\n" | slowcat | nc localhost 1337
    ;;
esac