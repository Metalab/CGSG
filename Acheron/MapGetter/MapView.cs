using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Drawing;
using System.Threading;
using System.Net;
using System.ComponentModel;

namespace MapGetter
{
    class MapView : Control
    {
        Image[] quadrants = new Image[4];

        int currentZoom = 1;

        PointF coordinates;
        int targetZoom;
        int targetResolution;

        [Category("Data"), Browsable(true), ReadOnly(false)]
        public PointF Coordinates
        {
            get { return coordinates; }
            set
            {
                coordinates = value;
                Invalidate();
            }
        }

        [Category("Data"), Browsable(true), ReadOnly(false)]
        public int TargetZoom
        {
            get { return targetZoom; }
            set
            {
                targetZoom = value;
                Invalidate();
            }
        }

        [Category("Data"), Browsable(true), ReadOnly(false)]
        public int TargetResolution
        {
            get { return targetResolution; }
            set
            {
                targetResolution = value;
                Invalidate();
            }
        }

        public MapView()
        {
            for (int i = 0; i < 4; i++)
            {

                //create dummy images
                quadrants[i] = new Bitmap(256, 256);
                using (var g = Graphics.FromImage(quadrants[i]))
                {
                    g.Clear(Color.BlueViolet);
                }
                int j = i;
                ThreadPool.QueueUserWorkItem((state) =>
                {
                    //start to load the map async
                    var req = WebRequest.Create("http://ecn.t3.tiles.virtualearth.net/tiles/a" + j + ".jpeg?g=0");
                    req.BeginGetResponse(HandleImageLoaded, new KeyValuePair<int, WebRequest>(j, req));
                });
            }
            Coordinates = new PointF(1.090867f, 0.464340f);
        }

        void HandleImageLoaded(IAsyncResult ar)
        {
            var para = (KeyValuePair<int, WebRequest>)ar.AsyncState;

            try
            {
                var resp = para.Value.EndGetResponse(ar);
                quadrants[para.Key] = Image.FromStream(resp.GetResponseStream());
                BeginInvoke(new Action(Invalidate));
            }
            catch (Exception ex)
            {
                BeginInvoke(new Action(() => MessageBox.Show(ex.ToString())));
            }
        }

        protected override void OnPaint(PaintEventArgs e)
        {
            var rect = new Rectangle(0, 0, Width / 2, Height / 2);
            var srect = new Rectangle(0,0,256,256);
            e.Graphics.DrawImage(quadrants[0], rect, srect, GraphicsUnit.Pixel);

            rect.X = Width / 2;
            e.Graphics.DrawImage(quadrants[1], rect, srect, GraphicsUnit.Pixel);

            rect.X = 0;
            rect.Y = Height / 2;
            e.Graphics.DrawImage(quadrants[2], rect, srect, GraphicsUnit.Pixel);

            rect.X = Width / 2;
            e.Graphics.DrawImage(quadrants[3], rect, srect, GraphicsUnit.Pixel);

            if (coordinates.X == -1)
            {

            }
            else
            {
                var transformedPoint = new PointF(coordinates.X * Width / 2, coordinates.Y * Height / 2);

                if (targetZoom >= 1 && targetZoom <= 19 && targetResolution > 0)
                {
                    var scaledResolution = targetResolution / Math.Pow(2, targetZoom - currentZoom);
                    rect = new Rectangle((int)Math.Floor(transformedPoint.X - scaledResolution / 2.0), (int)Math.Floor(transformedPoint.Y - scaledResolution / 2.0), (int)Math.Ceiling(scaledResolution), (int)Math.Ceiling(scaledResolution));
                    e.Graphics.DrawRectangle(new Pen(Color.Green, 2), rect);
                }
                else
                {
                    var pointRect = new RectangleF(transformedPoint.X - 3, transformedPoint.Y - 3, 6, 6);
                    e.Graphics.FillEllipse(new SolidBrush(Color.Red), pointRect);
                }
            }
        }

        protected override void OnPaintBackground(PaintEventArgs pevent)
        {
            
        }

        protected override void OnResize(EventArgs e)
        {
            Invalidate();
        }
    }
}
