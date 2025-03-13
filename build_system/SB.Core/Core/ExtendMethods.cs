using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace SB.Core
{
    public static class ExtendMethods
    {
        public static TParam Override<TKey, TParam>(this IDictionary<TKey, object?> dict, TKey key, TParam Value)
        {
            if (dict.TryGetValue(key, out object val))
                dict[key] = Value;
            else
                dict.Add(key, Value);
            return Value;
        }
        
        public static TParm GetOrAddNew<TKey, TParm>(this IDictionary<TKey, object?> dict, TKey key)
            where TParm: new()
        {
            object val;
            if (!dict.TryGetValue(key, out val))
                val = dict[key] = new TParm();
            return (TParm)val;
        }
        
        public static ICollection<T> AddRange<T>(this ICollection<T> @this, IEnumerable<T> items)
        {
            foreach (var item in items)
            {
                @this.Add(item);
            }
            return @this;
        }
    }
}
