PKG_CONFIG?=pkg-config
WAYLAND_PROTOCOLS!=$(PKG_CONFIG) --variable=pkgdatadir wayland-protocols
WAYLAND_SCANNER!=$(PKG_CONFIG) --variable=wayland_scanner wayland-scanner

CC=gcc
PKGS="wlroots-0.18" wayland-server xkbcommon
CFLAGS_PKG_CONFIG!=$(PKG_CONFIG) --cflags $(PKGS)
CFLAGS+=$(CFLAGS_PKG_CONFIG)
LIBS!=$(PKG_CONFIG) --libs $(PKGS)

SRC_DIR = src
BUILD_DIR = build
INCLUDE_DIR = $(SRC_DIR)/include
SRC_FILES = $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRC_FILES))

all: tnwlserver

# wayland-scanner is a tool which generates C headers and rigging for Wayland
# protocols, which are specified in XML. wlroots requires you to rig these up
# to your build system yourself and provide them in the include path.
$(INCLUDE_DIR)/xdg-shell-protocol.h:
	$(WAYLAND_SCANNER) server-header $(WAYLAND_PROTOCOLS)/stable/xdg-shell/xdg-shell.xml $@

$(SRC_DIR)/xdg-shell-protocol.c:
	$(WAYLAND_SCANNER) private-code $(WAYLAND_PROTOCOLS)/stable/xdg-shell/xdg-shell.xml $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(INCLUDE_DIR)/xdg-shell-protocol.h $(SRC_DIR)/xdg-shell-protocol.c
	@mkdir -p $(BUILD_DIR)
	$(CC) -c $< $(CFLAGS) -I./$(INCLUDE_DIR) -o $@

tnwlserver: $(OBJ_FILES)
	$(CC) $^ $(CFLAGS) $(LIBS) -o $@

clean:
	rm -rf $(BUILD_DIR) tnwlserver

.PHONY: all clean
