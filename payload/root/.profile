# /root/.profile — root login shell initialisation
# Sourced by bash for login shells (argv[0] starts with '-').

# Write sentinel first so we know .profile was reached regardless of what follows.
printf '%s\n' "[.profile] profile reached pid=$$" >> /root/bash_started
echo "[.profile] login shell initialised — BlueyOS root session"

# Source .bashrc for interactive login shells (standard pattern).
[ -f /root/.bashrc ] && . /root/.bashrc
