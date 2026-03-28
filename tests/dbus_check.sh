#!/bin/bash

echo "--- Kiriflux DBus Diagnostics ---"

# 1. Check if the service is registered
echo "Checking for org.kde.kiriflux service..."
dbus-send --session --dest=org.freedesktop.DBus --type=method_call --print-reply /org/freedesktop/DBus org.freedesktop.DBus.ListNames | grep kiriflux

# 2. Check for Menubar exportation
echo "Checking for Menubar object..."
dbus-send --session --dest=org.kde.kiriflux --type=method_call --print-reply /MenuBar/1 org.freedesktop.DBus.Introspectable.Introspect 2>/dev/null || echo "Menubar object not found at /MenuBar/1"

# 3. Check KAboutData via DBus (if exported)
echo "Checking for KAboutData interface..."
dbus-send --session --dest=org.kde.kiriflux --type=method_call --print-reply /MainApplication org.freedesktop.DBus.Introspectable.Introspect 2>/dev/null || echo "KAboutData object not found at /MainApplication"

echo "---------------------------------"
