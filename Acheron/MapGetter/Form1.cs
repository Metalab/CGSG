using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.IO;
using System.Reflection;
using System.Net;
using System.Threading;
using System.Drawing.Imaging;

namespace MapGetter
{
    public partial class Form1 : Form
    {
        Random rand = new Random();
        string cacheDir;

        public Form1()
        {
            InitializeComponent();

            //create map cache dir
            cacheDir = Path.Combine(Path.GetDirectoryName(Assembly.GetExecutingAssembly().CodeBase.Remove(0, 8)), "mapcache");
            Directory.CreateDirectory(cacheDir);
        }

        private void HandleCoordinatesChanged(object sender, EventArgs e)
        {
            try
            {
                var lat = float.Parse(tbLat.Text);
                var lon = float.Parse(tbLong.Text);

                var projection = new LiveMapsProjection(1);

                double x, y;
                projection.ToPixels(lat, lon, out x, out y);

                mapView1.Coordinates = new PointF((float)(x / (LiveMapsProjection.TileSize)),
                    (float)(y / (LiveMapsProjection.TileSize)));
            }
            catch
            {
                mapView1.Coordinates = new PointF(-1, -1);
            }

        }

        private void HandleTargetAreaChange(object sender, EventArgs e)
        {
            try
            {
                var targetRes = int.Parse(tbResolution.Text);
                var targetZoom = int.Parse(tbStartZoom.Text);

                mapView1.TargetResolution = targetRes;
                mapView1.TargetZoom = targetZoom;
            }
            catch
            {
                mapView1.TargetZoom = -1;
                mapView1.TargetResolution = -1;
            }
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            HandleCoordinatesChanged(this, EventArgs.Empty);
            HandleTargetAreaChange(this, EventArgs.Empty);
        }

        void SetStatus(int zoomLevel, int tileCount, int tilesFinished)
        {
            labStatus.Text = "Lvl " + zoomLevel;
            pbStatus.Minimum = 0;
            pbStatus.Maximum = tileCount;
            pbStatus.Value = tilesFinished;
        }

        private void btnGenerate_Click(object sender, EventArgs e)
        {
            btnGenerate.Enabled = tbLat.Enabled = tbLong.Enabled = tbResolution.Enabled = tbStartZoom.Enabled = false;

            //get parameters
            double lat, lon;
            int startZoom, res;
            try
            {
                lat = double.Parse(tbLat.Text);
                lon = double.Parse(tbLong.Text);
                startZoom = int.Parse(tbStartZoom.Text);
                res = int.Parse(tbResolution.Text);
            }
            catch
            {
                MessageBox.Show("parameters invalid");
                return;
            }

            //get target folder
            var folderDlg = new FolderBrowserDialog();
            if (folderDlg.ShowDialog() != DialogResult.OK) return;
            string folder = folderDlg.SelectedPath;

            ThreadPool.QueueUserWorkItem((state) =>
            {
                try
                {
                    //compute pixel coordinates for the closest zoom level
                    var projection = new LiveMapsProjection(19);
                    double cx, cy;
                    projection.ToPixels(lat, lon, out cx, out cy);
                    long px, py;
                    px = (long)Math.Floor(cx) - res / 2;
                    py = (long)Math.Floor(cy) - res / 2;

                    var img = ExtractImage(px, py, res, res, 19);
                    img.Save(Path.Combine(folder, "19.jpg"), ImageFormat.Jpeg);


                    for (int zoom = 18; zoom >= startZoom; zoom--)
                    {
                        //compute coordinates for the next rect
                        px = px / 2 - res / 4;
                        py = py / 2 - res / 4;

                        img = ExtractImage(px, py, res, res, zoom);
                        img.Save(Path.Combine(folder, zoom + ".jpg"), ImageFormat.Jpeg);
                    }

                    BeginInvoke(new Action<Exception>(DownloadFinished), new object[] { null });
                }
                catch (Exception ex)
                {
                    BeginInvoke(new Action<Exception>(DownloadFinished), ex);
                }
            });
        }

        void DownloadFinished(Exception ex)
        {
            btnGenerate.Enabled = tbLat.Enabled = tbLong.Enabled = tbResolution.Enabled = tbStartZoom.Enabled = true;
            if (ex == null)
                MessageBox.Show("Download finished OK.");
            else
                MessageBox.Show("Download aborted with Exception: " + ex.ToString());
        }

        Image ExtractImage(long x, long y, int width, int height, int zoomLevel)
        {
            //compute tiles to download
            int nrTilesHorizontal = width / LiveMapsProjection.TileSize;
            int nrTilesVertical = height / LiveMapsProjection.TileSize;

            if (x % LiveMapsProjection.TileSize != 0) nrTilesHorizontal++;
            if (y % LiveMapsProjection.TileSize != 0) nrTilesVertical++;

            long startColumn = x / LiveMapsProjection.TileSize;
            long startRow = y / LiveMapsProjection.TileSize;

            //download tiles and stitch together the image
            Image img = new Bitmap(nrTilesHorizontal * LiveMapsProjection.TileSize, nrTilesVertical * LiveMapsProjection.TileSize);
            using (var g = Graphics.FromImage(img))
            {
                for (int col = 0; col < nrTilesHorizontal; col++)
                {
                    for (int row = 0; row < nrTilesVertical; row++)
                    {
                        using (Image tile = GetTile(startColumn + col, startRow + row, zoomLevel))
                            g.DrawImage(tile, LiveMapsProjection.TileSize * col, LiveMapsProjection.TileSize * row);
                    }
                    BeginInvoke(new Action<int, int, int>(SetStatus), zoomLevel, nrTilesHorizontal * nrTilesVertical, col*nrTilesVertical);
                }
            }

            //now cut out the part we need if necessary
            if (img.Width == width && img.Height == height) return img;

            Image result = new Bitmap(width, height);
            using (var g = Graphics.FromImage(result))
            {
                g.DrawImage(img, new Rectangle(0, 0, width, height), x % LiveMapsProjection.TileSize, y % LiveMapsProjection.TileSize, width, height, GraphicsUnit.Pixel);
            }
            img.Dispose();

            return result;
        }

        Image GetTile(long column, long row, int zoom)
        {
            //compute tile address
            var sb = new StringBuilder("a", zoom+6);
            long mask = 1 << (zoom - 1);

            for (; zoom > 0; zoom--)
            {
                int val = (row & mask) != 0 ? 2 : 0;
                val += (column & mask) != 0 ? 1 : 0;

                sb.Append(val);
                row <<= 1;
                column <<= 1;
            }

            //get the image
            var path = Path.Combine(cacheDir, sb.ToString() + ".png");
            if (File.Exists(path)) return Image.FromFile(path); 

            //retrieve the image from the server
            int startServer = rand.Next(0, 4);
            Exception lastEx = null;
            for (int i = 0; i < 4; i++)
            {
                
                try
                {
                    var req = WebRequest.Create("http://ecn.t" + startServer + ".tiles.virtualearth.net/tiles/" + sb.ToString() + ".jpeg?g=0");
                    var img = Image.FromStream(req.GetResponse().GetResponseStream());
                    img.Save(path, System.Drawing.Imaging.ImageFormat.Png);
                    return img;
                }
                catch (WebException ex)
                {
                    lastEx = ex;
                    System.Threading.Thread.Sleep(rand.Next(400, 1200));
                }
                startServer = (startServer+1)%4;
            }

            throw new Exception("Download failed on all servers.", lastEx);
        }
    }
}
