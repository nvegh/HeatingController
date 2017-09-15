using System;

namespace TempController.Web.Extensions
{
    public static class Extensions
    {
        public static string ToGoogleDateTimeString(this DateTime dt)
        {
            //Date(Year, Month, Day, Hours, Minutes, Seconds, Milliseconds)
            return $"Date({dt.Year}, {dt.Month-1}, {dt.Day}, {dt.Hour}, {dt.Minute}, {dt.Second})";
        }

        public static int[] ToGoogleDateTimeArray(this DateTime dt)
        {
            //new Date(d[0], d[1], d[2], d[3], d[4], d[5]);
            return new int[] {dt.Year, dt.Month-1, dt.Day, dt.Hour, dt.Minute, dt.Second};
        }
    }
}