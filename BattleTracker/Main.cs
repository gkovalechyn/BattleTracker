using BattleTracker.Functions;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace BattleTracker {
    class Main {
        private static Main instance = null;

        private StreamWriter writer;
        private FunctionManager functionManager;

        public static Main Instance {
            get {
                return Main.instance;
            }

        }

        private Main() {
            this.functionManager = new FunctionManager();
        }

        public static void init() {
            Main.instance = new Main();
        }


        public int handleFunction(string function, string[] args, StringBuilder output) {
            return this.functionManager.call(function, args, output);
        }

        
        public void initializeFile(string missionName) {
            JArray array;
        }
    }
}
