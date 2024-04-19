using UnityEngine;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace PimDeWitte.UnityMainThreadDispatcher.Assets.roboarm.Scripts
{
    [Serializable]
    internal class ServoMotor
    {
        private float maxRotationAngle;
        private float minRotationAngle;
        private float rotationAngle;
        private float rotationSpeed;
        private Transform transform;

        public float MaxRotationAngle
        {
            get { return maxRotationAngle; }
            set { maxRotationAngle = value; }
        }
        
        public float MinRotationAngle
        {
            get { return minRotationAngle; }
            set { minRotationAngle = value; }
        }

        public float RotationSpeed
        {
            get { return rotationSpeed; }
            set { rotationSpeed = value; }
        }

        public float RotationAngle
        {
            get
            {
                return 0;
            }
            set
            {

            }
        }

        public ServoMotor(string path, float rotationSpeed = Constants.defaultRotationSpeed) {
            this.rotationSpeed = rotationSpeed;
            this.transform = transform.Find(path);
        }
    }
}
