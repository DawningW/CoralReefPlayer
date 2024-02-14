-keep,includedescriptorclasses class cn.oureda.coralreefplayer.NativeMethods {
    native *;
}

-keepclassmembers class * implements cn.oureda.coralreefplayer.CoralReefPlayer$Callback {
    void onEvent(...);
    void onFrame(...);
}
