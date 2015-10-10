gcc src/simpleserver.c src/backtrace.c -rdynamic -g -o build/simpleserver -I/opt/c/simpleserver/include -I/opt/AppDynamics/Agents/appdynamics-sdk-native/sdk_lib/include -L/opt/AppDynamics/Agents/appdynamics-sdk-native/sdk_lib/lib -lappdynamics_native_sdk
if [ $? -eq 0 ]
then
   export LD_LIBRARY_PATH=/opt/AppDynamics/Agents/appdynamics-sdk-native/sdk_lib/lib
   build/simpleserver
fi

