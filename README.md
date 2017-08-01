# GLES30_ProgrammingGuide_NDK
C with NDK Android GLES Tutorial

## How to

1.Download or clone this project,inside the folder,create Android Studio Project

2.Add `JNI Folder` In AS,for AS Load `JNILibs` easily,my JNI Folder path is:`${Project}/app/jni`,Drag `Android.mk` `Application.mk`(in template) into JNI Folder,also add your Clang files into JNI Folder

3.Edit the `Android.mk` Files

```
LOCAL_PATH          := $(call my-dir)
                    //Locate Common Folder
SRC_PATH            := ../../.. 
COMMON_PATH         := $(SRC_PATH)/Common
COMMON_INC_PATH     := $(COMMON_PATH)/Include
COMMON_SRC_PATH     := $(COMMON_PATH)/Source

include $(CLEAR_VARS)
//Module Name
LOCAL_MODULE    := Hello_Triangle
LOCAL_CFLAGS    += -DANDROID


LOCAL_SRC_FILES := $(COMMON_SRC_PATH)/esShader.c \
                   $(COMMON_SRC_PATH)/esShapes.c \
                   $(COMMON_SRC_PATH)/esTransform.c \
                   $(COMMON_SRC_PATH)/esUtil.c \
                   $(COMMON_SRC_PATH)/Android/esUtil_Android.c \
                   //Your Clang Class 
                   $(LOCAL_PATH)/Hello_Triangle.c
                   
                   
                   

LOCAL_C_INCLUDES    := $(SRC_PATH) \
                       $(COMMON_INC_PATH)
                   
LOCAL_LDLIBS    := -llog -landroid -lEGL -lGLESv3

LOCAL_STATIC_LIBRARIES := android_native_app_glue

include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/native_app_glue)
```

4.Edit the `Application.mk`

5.Edit the `AndroidManifest.xml`

```
<?xml version="1.0" encoding="utf-8"?>
<manifest xmlns:android="http://schemas.android.com/apk/res/android"
    package="com.martinrgb.chapter2_hellotriangle">
        <application
            android:label="HelloTriangle"
            android:hasCode="false">
            <activity android:name="android.app.NativeActivity"
                android:label="HelloTriangle"
                android:theme="@android:style/Theme.NoTitleBar.Fullscreen"
                android:launchMode="singleTask"
                android:configChanges="orientation|keyboardHidden">
            
                //The value should be as same as LOCAL_MODULE in android.mk
                <meta-data android:name="android.app.lib_name"
                    android:value="Hello_Triangle" />
                <intent-filter>
                    <action android:name="android.intent.action.MAIN" />
                    <category android:name="android.intent.category.LAUNCHER" />
                </intent-filter>
            </activity>
        </application>
        <uses-feature android:glEsVersion="0x00030000"/>
        <uses-sdk android:minSdkVersion="18"/>
</manifest>
```

6.open terminal,direct the JNI Folder, `ndk-build`

7.in `build.gradle`
```
//This snippets will hide the c&mk files
sourceSets.main {
    jniLibs.srcDir 'libs'
    jni.srcDirs = []
}
```

8.CMD + R,run your project

## Further Reading

for C interact With Java,you can read these:

1.[Calling OpenGL from C on Android, Using the NDK](http://www.learnopengles.com/calling-opengl-from-android-using-the-ndk/)

2.With Gradle 0.7.2+ you can put your native libraries directly into `src/main/jniLibs` and it will work automatically. https://stackoverflow.com/a/22488155/1636584 https://stackoverflow.com/a/16993006/1636584

3.[How to define NDK_MODULE_PATH](https://stackoverflow.com/questions/8549691/how-to-specify-directory-for-ndk-module-path)

For Example in `Android.mk`,you need define the local static lib's path

```
LOCAL_STATIC_LIBRARIES := libpng
...
$(call import-module,third_party/libpng)
```

In `~/.bash_profile`,you need define `NDK_MODULE_PATH':

```
export NDK_MODULE_PATH=/Users/MartinRGB/Library/Android/sdk/ndk-bundle/sources
```

also you need download [libpng-android](https://github.com/julienr/libpng-android),`ndk-build` to complie,then put it into `${NDK_MODULE_PATH}/third_party`

at last,in JNI Folder,`ndk-build`

## Concept

VBO:

顶点缓冲对象VBO是在显卡存储空间中开辟出的一块内存缓存区，用于**存储顶点的各类属性信息，如顶点坐标，顶点法向量，顶点颜色数据**等。在渲染时，可以直接从VBO中取出顶点的各类属性数据，由于**VBO在显存而不是在内存**中，不需要从CPU传输数据，**处理效率更高**。


VAO:

VAO，是这样一种方式：把**对象信息直接存储在图形卡中**，而不是在当我们需要的时候传输到图形卡。这就意味着我们的应用程序不用将数据传输到图形卡或者是从图形卡输出，这样也就获得了 **额外的性能提升**。

VBO保存了一个模型的顶点属性信息，每次绘制模型之前需要绑定顶点的所有信息，当数据量很大时，重复这样的动作变得非常麻烦。VAO可以把这些所有的配置都存储在**一个对象**中，每次绘制模型时，只需要绑定这个**VAO对象**就可以了。

使用VAO并不难。我们不需要大量的glVertex调用，而是把顶点数据存储在数组中，然后放进VBO，最后在VAO中存储相关的状态。记住：VAO中并没有存储顶点的相关属性数据。OpenGL会在后台为我们完成其他的功能。

Map Buffer:

临时映射缓冲,将缓冲区对象存储映射到应用程序的地址空间，相较于`glBufferData`，优点：

1. 减少 App 内存占用
2. 映射缓冲区返回的是 GPU 存储缓冲区地址空间的直接指针，避免了重复步骤，实现更好的**更新性能**


