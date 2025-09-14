include $(TOPDIR)/rules.mk

PKG_NAME:=json_parser
PKG_VERSION:=1.0
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SECTION:=local
  CATEGORY:=local-feed pkgs
  TITLE:=C++ JSON parser demo (nlohmann/json)
  DEPENDS:=+libstdcpp   # keep only the runtime C++ stdlib
endef

PKG_BUILD_DEPENDS:=nlohmannjson

define Package/$(PKG_NAME)/description
A tiny C++11 tool that reads a JSON file, parses it with nlohmann::json,
and pretty-prints it.
endef

# --- PREPARE: create build dir and copy our sources/headers ---
define Build/Prepare
	$(INSTALL_DIR) $(PKG_BUILD_DIR)/src
	$(CP) $(CURDIR)/src/* $(PKG_BUILD_DIR)/src/
	$(INSTALL_DIR) $(PKG_BUILD_DIR)/include
	$(CP) -r $(CURDIR)/include/* $(PKG_BUILD_DIR)/include/
	$(Build/Patch)
endef

# Use C++11
TARGET_CXXFLAGS += -std=gnu++11

# --- COMPILE: build all .cpp under src/ and link a single binary ---
define Build/Compile
	$(TARGET_CXX) $(TARGET_CXXFLAGS) $(TARGET_LDFLAGS) \
		-I$(PKG_BUILD_DIR)/include \
		-o $(PKG_BUILD_DIR)/json_parser \
		$(PKG_BUILD_DIR)/src/*.cpp
endef

# --- INSTALL: drop the binary into /usr/bin ---
define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/json_parser $(1)/usr/bin/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
