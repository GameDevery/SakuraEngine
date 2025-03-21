using System.Text;
using System.Security.Cryptography;
using System.Collections.Concurrent;
using System.Diagnostics;
using System.Reflection;

namespace SB
{
    [AttributeUsage(AttributeTargets.Class, Inherited = true, AllowMultiple = true)]
    public abstract class DoctorAttribute : Attribute
    {
        public abstract bool Check();
        public abstract bool Fix();
    }

    public partial class BuildSystem
    {
        public static Task RunDoctors()
        {
            return Task.Run(() => {
                var Doctors = new ConcurrentBag<DoctorAttribute>();
                foreach (var Assembly in AppDomain.CurrentDomain.GetAssemblies())
                {
                    if (!Assembly.GetReferencedAssemblies().Any(A => A.Name == "SB.Core"))
                        continue;

                    foreach (var Type in Assembly.GetTypes())
                    {
                        var DoctorAttrs = Type.GetCustomAttributes<DoctorAttribute>();
                        foreach (var DoctorAttr in DoctorAttrs)
                        {
                            Doctors.Add(DoctorAttr);
                        }
                    }
                }
                Parallel.ForEach(Doctors, (Doctor) =>
                {
                    if (!Doctor.Check())
                    {
                        if (!Doctor.Fix())
                        {
                            throw new Exception("Doctor failed to fix the issue");
                        }
                    }
                });
            });
        }
    }
}