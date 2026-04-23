# /root/.bashrc — root interactive shell initialisation
# Sourced by bash for interactive non-login shells, and via .profile for login shells.

# Write sentinel file using only shell builtins (no command substitutions).
printf '%s\n' "[.bashrc] bash started pid=$$" > /root/bash_started

echo "[.bashrc] bash is running (pid=$$)"
echo "[.bashrc] TERM=${TERM:-<unset>}  HOME=${HOME:-<unset>}  SHELL=${SHELL:-<unset>}"

PS1='[root@blueyos]# '
export PATH=/bin:/sbin:/usr/bin:/usr/sbin
