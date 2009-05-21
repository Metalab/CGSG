using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Drawing;

namespace MapGetter
{
    /// <summary>
    /// Mercator project as used by google maps and microsoft live maps. 
    /// Information was taken from http://cfis.savagexi.com/2006/05/03/google-maps-deconstructed
    /// </summary>
    class LiveMapsProjection
    {
        public const int TileSize = 256;

        int zoomLevel;
        long tiles;
        long circumference;
        double radius;
        

        public LiveMapsProjection(int zoomLevel)
        {
            this.zoomLevel = zoomLevel;
            tiles = 1L << zoomLevel;
            circumference = TileSize * tiles;
            radius = circumference / (2 * Math.PI);
        }

        public void ToPixels(double lat, double lon, out double x, out double y) {
            lat = Math.PI * lat / 180;
            lon = Math.PI * lon / 180;

            x = (lon * radius) + circumference/2;
            y = radius / 2 * Math.Log((1 + Math.Sin(lat)) / (1 - Math.Sin(lat)))*-1+circumference/2;
        }

        public void ToLatLong(double x, double y, out double lat, out double lon)
        {
            //undo origin shift
            x -= circumference / 2;
            y -= circumference/2;
            y = -y;

            //compte lat and long
            lon = (x / radius) / Math.PI * 180;
            lat = (Math.PI / 2 - 2 * Math.Atan(Math.Exp(-1 * y / radius))) / Math.PI * 180;
        }
    }
}
