# /root/.profile — root login shell initialisation
# Sourced by bash for login shells (argv[0] starts with '-').
#
# Diagnostic: write a marker file so we know this code ran even if the
# terminal is not displaying output.

# Source .bashrc for interactive login shells (standard pattern).
[ -f /root/.bashrc ] && . /root/.bashrc

# Record that .profile was sourced.
mkdir -p /root
printf '[%s] .profile sourced (pid=%s tty=%s)\n' \
    "$(date 2>/dev/null || echo unknown)" \
    "$$" \
    "$(tty 2>/dev/null || echo unknown)" \
    >> /root/bash_started

echo "[root .profile] login shell initialised — BlueyOS root session"
echo "[root .profile] shell: $SHELL  home: $HOME  term: $TERM"
echo "[root .profile] uid=$(id 2>/dev/null || echo unknown)"
