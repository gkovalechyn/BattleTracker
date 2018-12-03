using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace BattleTracker {
    public class Exports {

        /// <summary>
        /// Gets called when arma starts up and loads all extension.
        /// It's perfect to load in static objects in a seperate thread so that the extension doesn't needs any seperate initalization
        /// </summary>
        /// <param name="output">The string builder object that contains the result of the function</param>
        /// <param name="outputSize">The maximum size of bytes that can be returned</param>
#if WIN64
        [DllExport("RVExtensionVersion", CallingConvention = CallingConvention.StdCall)]
#else
        [DllExport("_RVExtensionVersion@8", CallingConvention = CallingConvention.StdCall)]
#endif
        public static void RvExtensionVersion(StringBuilder output, int outputSize) {
            AssemblyName assemblyName = Assembly.GetExecutingAssembly().GetName();

            output.Append(assemblyName.Version.ToString());

            Main.init();
        }

        /// <summary>
        /// The entry point for the default callExtension command. ("extension" callExtension "function")
        /// </summary>
        /// <param name="output">The string builder object that contains the result of the function</param>
        /// <param name="outputSize">The maximum size of bytes that can be returned</param>
        /// <param name="function">The string argument that is used along with callExtension</param>
#if WIN64
        [DllExport("RVExtension", CallingConvention = CallingConvention.StdCall)]
#else
        [DllExport("_RVExtension@12", CallingConvention = CallingConvention.StdCall)]
#endif
        public static void RvExtension(StringBuilder output, int outputSize, [MarshalAs(UnmanagedType.LPStr)] string function) {
            Main.Instance.handleFunction(function, new string[0], output);
        }

        /// <summary>
        /// The entry point for the callExtensionArgs command. ("extension" callExtension ["function", [args]])
        /// </summary>
        /// 
        /// <param name="output">The string builder object that contains the result of the function</param>
        /// <param name="outputSize">The maximum size of bytes that can be returned</param>
        /// <param name="function">The string argument that is used along with callExtension</param>
        /// <param name="args">The args passed to callExtension as a string array</param>
        /// <param name="argsCount">The size of the string array args</param>
        /// <returns>The result code</returns>
#if WIN64
        [DllExport("RVExtensionArgs", CallingConvention = CallingConvention.StdCall)]
#else
        [DllExport("_RVExtensionArgs@20", CallingConvention = CallingConvention.StdCall)]
#endif
        public static int RvExtensionArgs(
            StringBuilder output,
            int outputSize,
            [MarshalAs(UnmanagedType.LPStr)] string function,
            [MarshalAs(UnmanagedType.LPArray, ArraySubType = UnmanagedType.LPStr, SizeParamIndex = 4)] string[] args,
            int argCount) {

            return Main.Instance.handleFunction(function, args, output);
        }

    }
}
