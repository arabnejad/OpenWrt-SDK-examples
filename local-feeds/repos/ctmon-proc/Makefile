include $(TOPDIR)/rules.mk

#------------------------------ Package identity ------------------------------
PKG_NAME:=ctmon-proc
PKG_VERSION:=1.0
PKG_RELEASE:=1
PKG_LICENSE:=MIT
PKG_MAINTAINER:=hamid <hamid.arabnejad@gmail.com>

# Where OpenWrt stages & compiles sources for this package
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

# Bring in OpenWrt packaging helpers
include $(INCLUDE_DIR)/package.mk

#-------------------------- opkg-visible information --------------------------
define Package/$(PKG_NAME)
  SECTION:=local
  CATEGORY:=local-feed pkgs
  TITLE:=Minimal conntrack monitor via /proc (no deps)
  DEPENDS:=+libc
endef

# Description shown by opkg/feeds UIs
define Package/$(PKG_NAME)/description
A tiny C tool that reads /proc/net/nf_conntrack (once or every N seconds) and
optionally filters output via substring matching. No libnetfilter-conntrack or
libmnl required.
endef

#------------------------------- Build: prepare -------------------------------
# Copy *everything* from ./src and ./include (if present) into PKG_BUILD_DIR,
# then apply any patches from ./patches via $(Build/Patch).
define Build/Prepare
	$(INSTALL_DIR) $(PKG_BUILD_DIR)/src
	$(INSTALL_DIR) $(PKG_BUILD_DIR)/include

	$(CP) $(CURDIR)/src/* $(PKG_BUILD_DIR)/src/ 2>/dev/null || true
	$(CP) -r $(CURDIR)/include/* $(PKG_BUILD_DIR)/include/ 2>/dev/null || true

	$(Build/Patch)
endef

#------------------------------- Build: compile -------------------------------
# Compile ALL C files in PKG_BUILD_DIR/src into one binary (ctmon-proc).
# $(TARGET_CC), $(TARGET_CFLAGS), $(TARGET_LDFLAGS) come from the OpenWrt toolchain.
define Build/Compile
	$(TARGET_CC) $(TARGET_CFLAGS) $(TARGET_LDFLAGS) \
		-std=gnu11 -O2 -Wall -Wextra -D_GNU_SOURCE \
		-I$(PKG_BUILD_DIR)/include \
		-o $(PKG_BUILD_DIR)/ctmon-proc \
		$(wildcard $(PKG_BUILD_DIR)/src/*.c)
endef

#--------------------------------- Install -----------------------------------
# Copy artifacts into the ipk root (denoted by $(1)):
#  - /usr/bin/ctmon-proc
define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ctmon-proc $(1)/usr/bin/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
