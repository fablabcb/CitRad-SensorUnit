# Attention for editors: You have to use tab-characters for command indentation!
# vim: set noexpandtab:

PROJECT_ROOT ?= $(shell pwd)
BUILD_DIR_X86                         := $(PROJECT_ROOT)/build/x86
BUILD_DIR_ANDROID                     := $(PROJECT_ROOT)/build/android

ANDROID_PLATFORM                      := 29
ANDROID_SDK_DIR                       := ~/coding/cross/android
ANDROID_NDK_DIR                       := ~/coding/cross/android/ndk/21.4.7075529
ANDROID_QT_DIR                        := ~/coding/cross/qt/5.15.2/android
ANDROID_ADB                           := $(ANDROID_SDK_DIR)/platform-tools/adb


# local install prefix
INSTALL_PREFIX                        = deploy$(TYPE)

PID                                   ?= $(P)
TYPE                                  ?= Debug
#TYPE                                 ?= Release

.PHONY: default
default: build_x86

# If you cannot buid the android stuff, try using another java-version. This works with openjdk-8
# but not with openjdk-23. Gradle/Groovy will fail with some obscure errors.

.PHONY: help
help:
	@echo "Help:"
	@echo ""
	@echo "$(MAKE) configure_[x86|android]"
	@echo "$(MAKE) build_[x86|android]"
	@echo "$(MAKE) clean_[x86|android]"
	@echo ""
	@echo "$(MAKE) apk                      - build the android apk (same as build_android)"
	@echo "$(MAKE) buildNdeploy             - build the apk and upload to phone"
	@echo "$(MAKE) deploy                   - upload the apk to phone (build if not there)"
	@echo "$(MAKE) debug                    - show phone USB logtrace"

x86: build_x86
android: build_android

$(BUILD_DIR_X86)/CMakeCache.txt: CMakeLists.txt
	$(MAKE) configure_x86 > /dev/null
$(BUILD_DIR_ANDROID)/CMakeCache.txt: CMakeLists.txt
	$(MAKE) configure_android > /dev/null

# CONFIGURE
#
configure: configure_x86

reconfigure_x86:
	rm -rf $(BUILD_DIR_X86)
	$(MAKE) configure_x86

reconfigure_android:
	rm -rf $(BUILD_DIR_ANDROID)
	$(MAKE) configure_android

.PHONY: configure_x86
configure_x86:
	install -d $(BUILD_DIR_X86)
	(cd $(BUILD_DIR_X86) && \
	 cmake \
	  -G "Ninja" \
	  -DCMAKE_BUILD_TYPE=$(TYPE) \
	  -DCMAKE_INSTALL_PREFIX=$(INSTALL_PREFIX) \
	  $(O) \
	  ../.. \
	)

.PHONY: configure_android
configure_android:
	install -d $(BUILD_DIR_ANDROID)
	(cd $(BUILD_DIR_ANDROID) && \
	 cmake \
	  -G "Ninja" \
	  -DANDROID_PLATFORM=$(ANDROID_PLATFORM) \
	  -DCMAKE_BUILD_TYPE=$(TYPE) \
	  -DCMAKE_INSTALL_PREFIX=$(INSTALL_PREFIX) \
	  -DQT_ANDROID_SDK_ROOT:PATH=$(ANDROID_SDK_DIR) \
	  -DCMAKE_PREFIX_PATH:PATH=$(ANDROID_QT_DIR)/lib/cmake \
	  -DANDROID_NDK:PATH=$(ANDROID_NDK_DIR) \
	  -DCMAKE_TOOLCHAIN_FILE:FILEPATH=$(ANDROID_NDK_DIR)/build/cmake/android.toolchain.cmake \
	  -DNO_TESTS=TRUE \
	  -DANDROID=TRUE \
	  --log-level=DEBUG \
	  $(O) \
	  ../.. \
	)

# BUILD
#
.PHONY: build_x86
build_x86: $(BUILD_DIR_X86)/CMakeCache.txt
	cmake -E chdir $(BUILD_DIR_X86) cmake --build . -- -j 16
	cmake -E chdir $(BUILD_DIR_X86) cmake --build . --target install

.PHONY: build_android
build_android: $(BUILD_DIR_ANDROID)/CMakeCache.txt
	cmake -E chdir $(BUILD_DIR_ANDROID) cmake --build . -- -j 16

# CLEAN
#
.PHONY: clean
clean: clean_x86 clean_android

.PHONY: clean_x86
clean_x86:
	cmake -E chdir $(BUILD_DIR_X86) cmake --build . --target clean

.PHONY: clean_android
clean_android:
	cmake -E chdir $(BUILD_DIR_ANDROID) cmake --build . --target clean

.PHONY: deploy
deploy:
	$(ANDROID_SDK_DIR)/platform-tools/adb install -d $(BUILD_DIR_ANDROID)/citrad.mobileCompanion-armeabi-v7a/build/outputs/apk/debug/citrad.mobileCompanion-armeabi-v7a-debug.apk

.PHONY: buildNdeploy
buildNdeploy:
	$(MAKE) build_android
	$(MAKE) deploy

.PHONY: connectHandy
connectHandy:
	$(ANDROID_APD) devices -l
	echo "if the device is not listed, go to setting->developer stuff and enable USB debugging"
	#$(ANDROID_SDK_DIR)/platform-tools/adb attach ZY323NL6NQ
	
.PHONY: monitor
monitor:
	#$(ANDROID_ADB) logcat -c
	$(ANDROID_ADB) logcat -T '$(date +"%m.%d %H:%M:%S:000")' *:D
.PHONY: monitorPid
monitorPid:
	$(ANDROID_ADB) logcat --pid=$(PID) *:D

viewAPK:
	unzip -l $(ANDROID_APK)
