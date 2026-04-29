#!/bin/bash

FILE=$1

gcc -E -P "$FILE" > "${FILE%.c}".i
