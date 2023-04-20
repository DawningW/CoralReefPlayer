@echo off
rem genWindowsMakefiles.cmd
cd liveMedia  
nmake /f liveMedia.mak  
cd ../groupsock  
nmake /f groupsock.mak  
cd ../UsageEnvironment  
nmake /f UsageEnvironment.mak  
cd ../BasicUsageEnvironment  
nmake /f BasicUsageEnvironment.mak  
cd ../testProgs  
nmake /f testProgs.mak  
cd ../mediaServer  
nmake /f mediaServer.mak
cd ..
