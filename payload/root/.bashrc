# /root/.bashrc — root interactive shell initialisation
# Sourced by bash for interactive non-login shells, and via .profile for
# login shells.
#
# Diagnostic: write a marker file so we know bash is running even if the
# terminal is not displaying output.

# Record that bash is running.
mkdir -p /root
printf '[%s] .bashrc sourced (pid=%s tty=%s login=%s)\n' \
    "$(date 2>/dev/null || echo unknown)" \
    "$$" \
    "$(tty 2>/dev/null || echo unknown)" \
    "$( [ -o login_shell ] && echo yes || echo no )" \
    >> /root/bash_started

echo "[root .bashrc] bash is running (pid=$$, tty=$(tty 2>/dev/null || echo ?))"
echo "[root .bashrc] TERM=${TERM:-<unset>}  HOME=${HOME:-<unset>}  SHELL=${SHELL:-<unset>}"

# Basic prompt.
PS1='[\u@\h \W]\$ '

export PATH=/bin:/sbin:/usr/bin:/usr/sbin
