# This file should be sourced from your .zshrc file.
#
# Copyright 2016-2019 Simon Edwards <simon@simonzone.com>
#
# This source code is licensed under the MIT license which is detailed in the LICENSE.txt file.
#

if [ -z "$LC_EXTRATERM_COOKIE" ]; then
    return 0
fi
if [[ "$TERM" =~ "screen" ]]; then
    unset -v LC_EXTRATERM_COOKIE
    return 0
fi

if [ ! -t 0 ] ; then
    # Not an interactive terminal
    unset -v LC_EXTRATERM_COOKIE
    return 0
fi

autoload -Uz add-zsh-hook


echo "Setting up Extraterm support."

# Put our enhanced commands at the start of the PATH.
filedir=`dirname "${(%):-%x}"`
if [ "${filedir:0:1}" = "/" ]
then
    export PATH="$filedir:$PATH"
else
    export PATH="$PWD/$filedir:$PATH"
fi

# Insert our special code to communicate to Extraterm the status of the last command.
extraterm_install_prompt_integration () {
    local prefix

    if [[ ! "$PS1" =~ "$LC_EXTRATERM_COOKIE" ]] ; then
        prefix=`echo -n -e "%{\0033&${LC_EXTRATERM_COOKIE};3\0007%}%?%{\0000%}"`
        export PS1="%{${prefix}%}${PS1}"
    fi
}
extraterm_install_prompt_integration
add-zsh-hook precmd extraterm_install_prompt_integration

preexec () {
    echo -n -e "\0033&${LC_EXTRATERM_COOKIE};2;zsh\0007"
    echo -n $1
    echo -n -e "\0000"
}

if [[ "$(uname)" == 'Linux' ]];
then
  if [[ "$(uname -m)" == 'aarch64' ]];
  then
    alias from="from.aarch64-linux-musl"
    alias show="show.aarch64-linux-musl"
  else
    alias from="from.x86_64-linux-musl"
    alias show="show.x86_64-linux-musl"
  fi
fi

if [[ "$(uname)" == 'Darwin' ]]; then
  alias from="from.x86_64-macos"
  alias show="show.x86_64-macos"
fi
