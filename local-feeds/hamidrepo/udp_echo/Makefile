# Pull in core build rules (toolchain, $(TOPDIR), $(INCLUDE_DIR), etc.)
include $(TOPDIR)/rules.mk

#------------------------------ Package identity ------------------------------
PKG_NAME:=udp_echo
PKG_VERSION:=1.0
PKG_RELEASE:=1
PKG_LICENSE:=MIT
PKG_MAINTAINER:=hamid <hamid.arabnejad@gmail.com>

# Where OpenWrt stages & compiles sources for this package
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

# Bring in packaging helpers (defines BuildPackage, install macros, etc.)
include $(INCLUDE_DIR)/package.mk

#-------------------------- opkg-visible information --------------------------
define Package/udp_echo
  SECTION:=net
  CATEGORY:=Network
  TITLE:=UDP echo daemon
  DEPENDS:=+libc
endef

# Description shown by opkg/feeds UIs
define Package/$(PKG_NAME)/description
A tiny C UDP echo daemon using epoll. Listens on a configurable address/port and
echoes incoming UDP datagrams back to the sender. Optional -t flag can prefix
replies with a UTC timestamp.
endef

#------------------------------- Build: prepare -------------------------------
# Copy *everything* from ./src and ./include (if present) into PKG_BUILD_DIR,
# then apply any patches from ./patches via $(Build/Patch).
define Build/Prepare
	# Ensure build subdirs exist
	$(INSTALL_DIR) $(PKG_BUILD_DIR)/src
	$(INSTALL_DIR) $(PKG_BUILD_DIR)/include

	# Copy sources (ignore error if ./src does not exist or is empty)
	$(CP) $(CURDIR)/src/* $(PKG_BUILD_DIR)/src/ 2>/dev/null || true

	# Copy headers (ignore error if ./include does not exist or is empty)
	$(CP) -r $(CURDIR)/include/* $(PKG_BUILD_DIR)/include/ 2>/dev/null || true

	# Apply patches from $(CURDIR)/patches (quilt-generated, if any)
	$(Build/Patch)
endef

#------------------------------- Build: compile -------------------------------
# Compile ALL C files in PKG_BUILD_DIR/src into one binary (udp_echo).
# $(TARGET_CC), $(TARGET_CFLAGS), $(TARGET_LDFLAGS) come from the OpenWrt toolchain.
define Build/Compile
	$(TARGET_CC) $(TARGET_CFLAGS) \
		-std=gnu11 -O2 -Wall -Wextra -D_GNU_SOURCE \
		-I$(PKG_BUILD_DIR)/include \
		$(PKG_BUILD_DIR)/src/*.c \
		$(TARGET_LDFLAGS) \
		-o $(PKG_BUILD_DIR)/udp_echo
endef


#--------------------------------- Install -----------------------------------
# Copy artifacts into the ipk root (denoted by $(1)):
#  - /usr/bin/udp_echo                  (binary)
#  - /etc/init.d/udp-echo               (procd init script)
#  - /etc/config/udp_echo               (UCI config)
#  - /etc/uci-defaults/99-udp-echo      (first-boot defaults)
define Package/$(PKG_NAME)/install
	# Binary
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/udp_echo $(1)/usr/bin/

	# Init script (procd)
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(CURDIR)/scripts/etc/init.d/udp-echo $(1)/etc/init.d/

	# UCI config
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) $(CURDIR)/scripts/etc/config/udp_echo $(1)/etc/config/

	# First-boot defaults
	$(INSTALL_DIR) $(1)/etc/uci-defaults
	$(INSTALL_BIN) $(CURDIR)/scripts/etc/uci-defaults/99-udp-echo $(1)/etc/uci-defaults/
endef

$(eval $(call BuildPackage,udp_echo))
