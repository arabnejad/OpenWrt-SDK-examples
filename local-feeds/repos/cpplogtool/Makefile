include $(TOPDIR)/rules.mk

PKG_NAME:=cpplogtool
PKG_RELEASE:=1
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

PKG_LICENSE:=MIT
PKG_MAINTAINER:=hamid.arabnejad@gmail.com


include $(INCLUDE_DIR)/package.mk

define Package/cpplogtool
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=Simple router log top-talkers tool
  DEPENDS:=+libstdcpp
endef

define Package/cpplogtool/description
 Minimal C++11 CLI to parse netfilter-style logs and print top talkers by SRC IP.
endef

# Copy sources into a throwaway build dir
define Build/Prepare
	$(call Build/Prepare/Default)
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) $(CURDIR)/src $(PKG_BUILD_DIR)/
	$(CP) $(CURDIR)/include $(PKG_BUILD_DIR)/
	$(CP) $(CURDIR)/Makefile.build $(PKG_BUILD_DIR)/
endef

# Compile using your helper makefile with the target toolchain
define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) -f Makefile.build \
		CXX="$(TARGET_CXX)" \
		CXXFLAGS="$(TARGET_CXXFLAGS) -std=c++11 -Os -Wall -Wextra -Werror -Iinclude" \
		LDFLAGS="$(TARGET_LDFLAGS)"
endef

# Install the produced binary
define Package/cpplogtool/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/cpplogtool $(1)/usr/bin/
endef

$(eval $(call BuildPackage,cpplogtool))
