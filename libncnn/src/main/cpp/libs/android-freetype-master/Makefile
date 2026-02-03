.PHONY: default
default:
	cd Android && ndk-build

.PHONY: clean
clean:
	cd Android && rm -Rf libs/ obj/

.PHONY: build
build: clean default
	cp -f Android/libs/armeabi-v7a/libfreetype.so \
	  $(CASTLE_ENGINE_PATH)/tools/build-tool/data/android/integrated-services/freetype/app/src/main/jniLibs/armeabi-v7a/libfreetype.so
	cp -f Android/libs/arm64-v8a/libfreetype.so \
	  $(CASTLE_ENGINE_PATH)/tools/build-tool/data/android/integrated-services/freetype/app/src/main/jniLibs/arm64-v8a/libfreetype.so
	cp -f Android/libs/x86/libfreetype.so \
	  $(CASTLE_ENGINE_PATH)/tools/build-tool/data/android/integrated-services/freetype/app/src/main/jniLibs/x86/libfreetype.so
	cp -f Android/libs/x86_64/libfreetype.so \
	  $(CASTLE_ENGINE_PATH)/tools/build-tool/data/android/integrated-services/freetype/app/src/main/jniLibs/x86_64/libfreetype.so
