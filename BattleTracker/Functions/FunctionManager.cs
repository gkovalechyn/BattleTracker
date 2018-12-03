using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace BattleTracker.Functions {
    class FunctionManager {
        private readonly Dictionary<string, Function> functionMap = new Dictionary<string, Function>();

        public FunctionManager() {

        }

        public int call(string function, string[] args, StringBuilder output) {
            if (this.functionMap.ContainsKey(function)) {
                return this.functionMap[function].execute(args, output);
            } else {
                output.Append("FAIL");
                return 0;
            }
        }
    }
}
