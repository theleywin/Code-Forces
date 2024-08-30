#!/bin/bash

while [ 1 == 1 ]; do
  clear
  echo " "
  echo " "
  echo " "
  echo " "
  echo " "
  echo " "
  echo " "
  echo " "
  echo " "
  echo "                                                                    SPACE INVADER                  "
  echo " "
  echo " "
  echo " "
  echo "                                                            |￣￣￣￣￣￣￣￣￣￣￣￣￣￣|             "
  echo "                                                                   Welcome to                     "
  echo "                                                                  the adventure                    "
  echo "                                                            |＿＿＿＿＿＿＿＿＿＿＿＿＿＿|              "
  echo "                                                                    \ (•◡•) /                        "
  echo "                                                                     \     /                         "
  echo " "
  echo " "
  echo " "
  echo "                                                                  By Diego & Pablo"
  echo " "
  echo " "
  echo "[s] -> Start the adventure"
  echo "[e] -> Exit (just for loosers)"
  echo " "
  echo -n "> "
  read input
  if [ $input == "e" ] ; then
    break
  elif [ $input == "s" ]; then
    ./rungame
  fi
done