namespace SB.Core
{
    public interface IArgumentList
    {
        public void Merge(IArgumentList Another);
        public IArgumentList Copy();
    }

    public class ArgumentList<T> : List<T>, IArgumentList
    {
        public void Merge(IArgumentList Another)
        {
            if (Another is not List<T>)
                throw new ArgumentException("ArgumentList type mismatch!");
            var ToMerge = Another as List<T>;
            this.AddRange(ToMerge);
        }

        public IArgumentList Copy()
        {
            IArgumentList Cpy = new ArgumentList<T>();
            Cpy.Merge(this);
            return Cpy;
        }
    }
}
