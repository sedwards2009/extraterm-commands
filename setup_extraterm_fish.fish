# This file should be sourced from your ~/.config/config.fish file.
#
# Copyright 2014-2024 Simon Edwards <simon@simonzone.com>
#
# This source code is licensed under the MIT license which is detailed in the LICENSE.txt file.
#

set -l COMMAND_DIR (dirname (status -f))

if test -z "$LC_EXTRATERM_COOKIE"
  exit 0
end

if echo "$TERM" | grep -q -E '^screen'
  set -e LC_EXTRATERM_COOKIE
  exit 0
end

if not isatty
  set -e LC_EXTRATERM_COOKIE
  exit 0
end

if not status is-interactive
  exit 0
end

echo "Setting up Extraterm support."

function extraterm_preexec -e fish_preexec
  echo -n -e -s "\033&" $LC_EXTRATERM_COOKIE ";2;fish\007"
  echo -n $argv[1]
  echo -n -e "\000"
end

function extraterm_postexec -e fish_postexec
  set -l status_backup $status
  echo -n -e -s "\033" "&" $LC_EXTRATERM_COOKIE ";3\007"
  echo -n $status_backup
  echo -n -e "\000"
end

set -l BINARY_SUFFIX ""
if test (uname) = 'Linux'
  if test (uname -m) = 'aarch64'
    set BINARY_SUFFIX aarch64-linux-musl
  else
    set BINARY_SUFFIX x86_64-linux-musl
  end
else
  if test (uname) = 'Darwin'
    set BINARY_SUFFIX x86_64-macos
  end
end

if test "$BINARY_SUFFIX" != ""
  eval function\ from\n\"$COMMAND_DIR/from.$BINARY_SUFFIX\" \$argv\nend\n
  eval function\ show\n\"$COMMAND_DIR/show.$BINARY_SUFFIX\" \$argv\nend\n
end
