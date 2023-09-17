
/* Developed By Hamid.Memar         */
/* License : MIT                    */

using System.Runtime.InteropServices;

namespace RemoteMXS.SDK.Managed
{
    public static class RemoteMaxScriptAPI
    {
        // Regular Imports
        [DllImport("RemoteMXS.SDK.dll")] extern public static bool InitializeRemoteMXSEngineSDK();
        [DllImport("RemoteMXS.SDK.dll")] extern public static int GetFirst3dsMaxInstanceProcessID();
        [DllImport("RemoteMXS.SDK.dll")] extern public static bool ValidateRemote3dsMaxConnection(int maxPID);
        [DllImport("RemoteMXS.SDK.dll")] extern public static bool InitializeRemote3dsMaxConnection(int maxPID);
        [DllImport("RemoteMXS.SDK.dll")] extern public static bool ReleaseRemote3dsMaxConnection();

        // Special Imports
        [DllImport("RemoteMXS.SDK.dll", CharSet = CharSet.Unicode)]
        extern public static bool ExecuteRemoteMaxScript([MarshalAs(UnmanagedType.LPWStr)] string maxscriptScript);
    }
}