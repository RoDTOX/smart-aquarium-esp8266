#!/usr/bin/env python3
import sys

watchdog_path = "/data/data/com.termux/files/home/watchdog.sh"

with open(watchdog_path, "r", encoding="utf-8") as f:
    lines = f.readlines()

new_lines = []
skip = False

new_grafana_block = [
    '    # --- 5. GRAFANA HEALTH CHECK (PORT 3000) ---\n',
    '    HTTP_GF=$(curl -s -o /dev/null -w "%{http_code}" --max-time 5 http://127.0.0.1:3000/grafana/api/health 2>/dev/null)\n',
    '    if [ "$HTTP_GF" != "200" ]; then\n',
    '        HTTP_GF_ROOT=$(curl -s -o /dev/null -w "%{http_code}" --max-time 5 http://127.0.0.1:3000/ 2>/dev/null)\n',
    '        if [ "$HTTP_GF_ROOT" != "200" ] && [ "$HTTP_GF_ROOT" != "301" ] && [ "$HTTP_GF_ROOT" != "302" ] && [ "$HTTP_GF_ROOT" != "401" ]; then\n',
    '            FAIL_COUNT_GRAFANA=$((FAIL_COUNT_GRAFANA + 1))\n',
    '            if [ "$FAIL_COUNT_GRAFANA" -ge 4 ]; then\n',
    '                log_msg "[REPAIR] Grafana dead for 4 checks. Restarting Grafana in tmux..."\n',
    '                tmux kill-session -t grafana 2>/dev/null\n',
    '                tmux new-session -d -s grafana "proot-distro login debian -- /usr/share/grafana/bin/grafana-server --config=/etc/grafana/grafana.ini --homepath=/usr/share/grafana cfg:default.paths.logs=/var/log/grafana cfg:default.paths.data=/var/lib/grafana"\n',
    '                FAIL_COUNT_GRAFANA=0\n',
    '            fi\n',
    '        else\n',
    '            FAIL_COUNT_GRAFANA=0\n',
    '        fi\n',
    '    else\n',
    '        FAIL_COUNT_GRAFANA=0\n',
    '    fi\n'
]

i = 0
found = False
while i < len(lines):
    line = lines[i]
    if "# --- 5. GRAFANA HEALTH CHECK" in line:
        found = True
        new_lines.extend(new_grafana_block)
        # Skip until section 6
        while i < len(lines) and "# --- 6." not in lines[i]:
            i += 1
        continue
    new_lines.append(line)
    i += 1

if found:
    with open(watchdog_path, "w", encoding="utf-8") as f:
        f.writelines(new_lines)
    print("SUCCESS: watchdog.sh updated.")
else:
    print("ERROR: Grafana section not found.")
