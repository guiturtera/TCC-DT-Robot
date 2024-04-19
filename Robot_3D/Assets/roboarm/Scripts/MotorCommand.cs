using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace PimDeWitte.UnityMainThreadDispatcher.Assets.roboarm.Scripts
{
    [Serializable]
    public class MotorCommand
    {
        public string type = "command";
        public float value;

        public MotorCommand(float value) {
            this.value = value;
        }
    }
}
