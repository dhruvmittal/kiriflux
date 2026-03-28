#!/bin/bash

echo "--- Kiriflux DBus Tree Scan ---"

# List all objects under org.kde.kiriflux
echo "Objects for org.kde.kiriflux:"
dbus-send --session --dest=org.freedesktop.DBus --type=method_call --print-reply /org/freedesktop/DBus org.freedesktop.DBus.ListNames | grep kiriflux

# Scan objects recursively (depth 3)
function scan_path() {
    local path=$1
    echo "Path: $path"
    dbus-send --session --dest=org.kde.kiriflux --type=method_call --print-reply "$path" org.freedesktop.DBus.Introspectable.Introspect 2>/dev/null | grep '<node name=' | sed 's/.*name="\([^"]*\)".*/\1/' | while read -r node; do
        local subpath="$path"
        [[ "$subpath" != "/" ]] && subpath="$subpath/"
        subpath="$subpath$node"
        scan_path "$subpath"
    done
}

scan_path "/"

echo "-------------------------------"
