using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.IO.Ports;
using System.IO;
using System.Windows.Threading;
using System.ComponentModel;
using System.Globalization;
using System.Collections.ObjectModel;
using System.Threading.Tasks;
using System.Diagnostics;

using OxyPlot;
using OxyPlot.Series;
using OxyPlot.Axes;
using OxyPlot.Annotations;

namespace Teplomer
{
    public class Graf
    {
        public enum ROZSAHY { PET_MINUT, HODINA, DVACETCTIRY_HODIN}
        public PlotModel DataPlot { get; set; }

        private int posunOsy; // posuv osy X, v sekundach
        private DateTime start5MinOsy;
        private DateTime start1HodOsy;
        private DateTime start24HodOsy;
        public Graf()
        {
            DataPlot = new PlotModel();

            // vytvoreni serie vykreslovanych bodu
            DataPlot.Series.Add(new LineSeries() { MarkerType = MarkerType.Diamond, MarkerSize = 4, MarkerFill = OxyColors.YellowGreen, MarkerStroke = OxyColors.DarkGreen, MarkerStrokeThickness = .5, TrackerFormatString = "{2:HH:mm:ss}: {4:0.00} °C", Background = OxyColors.WhiteSmoke });
            DataPlot.Series.Add(new LineSeries() { MarkerType = MarkerType.Diamond, MarkerSize = 4, MarkerFill = OxyColors.YellowGreen, MarkerStroke = OxyColors.DarkGreen, MarkerStrokeThickness = .5, TrackerFormatString = "{2:HH:mm:ss}: {4:0.00} °C", Background = OxyColors.WhiteSmoke });
            DataPlot.Series.Add(new LineSeries() { MarkerType = MarkerType.Diamond, MarkerSize = 4, MarkerFill = OxyColors.YellowGreen, MarkerStroke = OxyColors.DarkGreen, MarkerStrokeThickness = .5, TrackerFormatString = "{2:HH:mm:ss}: {4:0.00} °C", Background = OxyColors.WhiteSmoke });           
            
            // posun osy ... defaultne o 290 s (zobrazeni dat v grafu o delce 5 min)
            posunOsy = -300;

            DataPlot.Title = "Průběh teploty (5 min)";
            DataPlot.TitleColor = OxyColors.WhiteSmoke;
            DataPlot.Background = OxyColors.Transparent;

            // osa x
            OxyPlot.Axes.DateTimeAxis datetimeAxis1 = new OxyPlot.Axes.DateTimeAxis()
            {
                Position = AxisPosition.Bottom,
                StringFormat = "HH:mm",//Constants.MarketData.DisplayDateFormat,
                Title = "Čas  [hh:mm]",                
                TitleColor = OxyColors.WhiteSmoke,
                TickStyle = TickStyle.Inside,
                TextColor = OxyColors.WhiteSmoke,
                FontWeight = OxyPlot.FontWeights.Bold,
                TitleFontWeight = OxyPlot.FontWeights.Bold,
                AxisTitleDistance = 10,
                //IntervalLength = 60,
                //MinorIntervalType = OxyPlot.Axes.DateTimeIntervalType.Seconds,
                //IntervalType = OxyPlot.Axes.DateTimeIntervalType.Minutes,
                //IntervalType = OxyPlot.Axes.DateTimeIntervalType.Seconds,
                //MinorIntervalType = OxyPlot.Axes.DateTimeIntervalType.Milliseconds,

                //IntervalType = DateTimeIntervalType.Minutes,
                MajorGridlineStyle = LineStyle.Solid,
                MinorGridlineStyle = LineStyle.Solid,
                MajorGridlineColor = OxyColors.Black,
                MinorGridlineColor = OxyColors.Red,
                Minimum = OxyPlot.Axes.DateTimeAxis.ToDouble(DateTime.Now),
                Maximum = OxyPlot.Axes.DateTimeAxis.ToDouble(DateTime.Now.AddMinutes(5)),
                //AbsoluteMinimum = OxyPlot.Axes.DateTimeAxis.ToDouble(DateTime.Now.AddMinutes(-1)), //DateTime.Now.AddHours(-1)),
                //AbsoluteMaximum = OxyPlot.Axes.DateTimeAxis.ToDouble(DateTime.Now.AddMinutes(10)),

                //MinimumRange = 1e-3//DateTimeAxis.ToDouble(new DateTime(2015,7,24,01,0,10))
                IsZoomEnabled = false,
                IsPanEnabled = false,

                MaximumPadding = 0.05,
                MinimumPadding = 0.05
            };// DateTimeAxis()        
            //datetimeAxis1.AxisChanged += datetimeAxis1_AxisChanged;
            
            DataPlot.Axes.Add(datetimeAxis1);

            // osa y
            var linearAxis2 = new OxyPlot.Axes.LinearAxis()
            {
                Title = "Teplota [°C]",
                TitleColor = OxyColors.WhiteSmoke,
                TickStyle = TickStyle.Inside,
                TextColor = OxyColors.WhiteSmoke,
                FontWeight = OxyPlot.FontWeights.Bold,
                TitleFontWeight = OxyPlot.FontWeights.Bold,
                MajorGridlineStyle = LineStyle.Solid,
                MinorGridlineStyle = LineStyle.Dash,
                MajorGridlineColor = OxyColors.Black,
                MinorStep = 5,

                AxisTitleDistance = 8,
                IsPanEnabled = false,
                IsZoomEnabled = false,
                Selectable = false,
                Minimum = 10,
                Maximum = 40,
                AbsoluteMinimum = 0,
                AbsoluteMaximum = 50
            };
            DataPlot.Axes.Add(linearAxis2);

            DateTime cas = DateTime.Now; cas = cas.AddMilliseconds(-cas.Millisecond).AddSeconds(-cas.Second);
            
            start5MinOsy = cas.AddMinutes(-cas.Minute % 5);
            start1HodOsy = cas.AddMinutes(-cas.Minute).AddSeconds(-cas.Second);
            start24HodOsy = Process.GetCurrentProcess().StartTime;
        }

        public void pridejBod(DateTime t, double y)
        {
            double tim = DateTimeAxis.ToDouble(t);
            LineSeries linSer1 = (DataPlot.Series[0] as OxyPlot.Series.LineSeries);
            LineSeries linSer2 = (DataPlot.Series[1] as OxyPlot.Series.LineSeries);
            LineSeries linSer3 = (DataPlot.Series[2] as OxyPlot.Series.LineSeries);

            Dispatcher.CurrentDispatcher.Invoke((Action)(() =>
            {
                linSer1.Points.Add(new OxyPlot.DataPoint(tim, y));

            }));
            if (t > start5MinOsy.AddMinutes(5))
            {
                // zprumeruju pro zobrazeni za hodinu
                linSer2.Points.Add(new OxyPlot.DataPoint(tim, linSer1.Points.Average(x => x.Y)));
                start5MinOsy = start5MinOsy.AddMinutes(5);
                
                if (t > start1HodOsy.AddHours(1))
                {
                    linSer3.Points.Add(new OxyPlot.DataPoint(tim, linSer2.Points.Average(x => x.Y)));
                    linSer2.Points.RemoveRange(0, linSer2.Points.Count - 1);
                    start1HodOsy = start1HodOsy.AddHours(1);
                }
                
                if (t > start24HodOsy.AddHours(24))
                {
                    linSer3.Points.RemoveRange(0, linSer3.Points.Count - 1);
                    start24HodOsy = start24HodOsy.AddHours(24);
                }
                linSer1.Points.RemoveRange(0, linSer1.Points.Count - 1); // odstraneni bodu, ktere stejne nebudou videt
            }
            // posun osy X
            DateTime dt_AxisActMax = OxyPlot.Axes.DateTimeAxis.ToDateTime((DataPlot.Axes[0] as OxyPlot.Axes.DateTimeAxis).ActualMaximum);
            DateTime dt_AxisOffset = OxyPlot.Axes.DateTimeAxis.ToDateTime((DataPlot.Axes[0] as OxyPlot.Axes.DateTimeAxis).Offset);
            //DateTime dt_bod = DateTimeAxis.ToDateTime((linSer1.Points.Last()).X);
            if (linSer1.Points.Count > 0)//pokud bude zobrazeny aspon 1 bod
            {
                if (t > dt_AxisActMax)
                {
                    // zprumeruju pro zobrazeni za hodinu
                    /*linSer2.Points.Add(new OxyPlot.DataPoint(tim, linSer1.Points.Average(x => x.Y)));

                    if (linSer2.Points.Count >= 12)
                    {
                        linSer3.Points.Add(new OxyPlot.DataPoint(tim, linSer2.Points.Average(x => x.Y)));
                        linSer2.Points.RemoveRange(0, linSer2.Points.Count - 1);
                    }*/
                    (DataPlot.Axes[0] as OxyPlot.Axes.DateTimeAxis).IsPanEnabled = true; // musim povolit posun osy !!
                    
                    DateTime d10 = dt_AxisOffset.AddSeconds(posunOsy);// offset je cas zacatku osy X. pro zobrazeni grafu o delce 5 min jej posunu o 300 s vzad => novy zacatek bude vlastne aktualni cas konce osy

                    double off = DateTimeAxis.ToDouble(d10); // prevedeny cas do cisla
                    double panStep = (DataPlot.Axes[0] as DateTimeAxis).Transform(off); // vypocet hodnoty posunu
                    (DataPlot.Axes[0] as OxyPlot.Axes.DateTimeAxis).Pan(panStep); // posunuti osy X vzad
                    

                    (DataPlot.Axes[0] as OxyPlot.Axes.DateTimeAxis).IsPanEnabled = false; // a zase ho zakazat !!                   
                }
            }
            DataPlot.InvalidatePlot(true);
        }

        public void nastavRozsahOsyX(ROZSAHY rozsah)
        {
            DateTimeAxis osa = DataPlot.Axes[0] as DateTimeAxis;
            DateTime cas = DateTime.Now;
            cas = cas.AddMilliseconds(-cas.Millisecond).AddSeconds(-cas.Second);

            foreach(var v in DataPlot.Series)
            {
                v.IsVisible = false; 
            }
            switch (rozsah)
            {
                case ROZSAHY.PET_MINUT:
                    start5MinOsy = cas.AddMinutes(-cas.Minute % 5);
                    
                    osa.Minimum = DateTimeAxis.ToDouble(start5MinOsy);
                    osa.Maximum = DateTimeAxis.ToDouble(start5MinOsy.AddMinutes(5));
                    posunOsy = -300;
                    DataPlot.Title = "Průběh teploty (5 min)";
                    DataPlot.Series[0].IsVisible = true;
                    break;
                case ROZSAHY.HODINA:
                    start1HodOsy = cas.AddMinutes(-cas.Minute).AddSeconds(-cas.Second);

                    osa.Minimum = DateTimeAxis.ToDouble(start1HodOsy);
                    osa.Maximum = DateTimeAxis.ToDouble(start1HodOsy.AddHours(1));
                    posunOsy = -3600;
                    DataPlot.Title = "Průběh teploty (1 hod)";
                    DataPlot.Series[1].IsVisible = true;
                    break;
                case ROZSAHY.DVACETCTIRY_HODIN:
                    start24HodOsy = Process.GetCurrentProcess().StartTime;
                    start24HodOsy = start24HodOsy.AddMilliseconds(-start24HodOsy.Millisecond).AddSeconds(-start24HodOsy.Second).AddMinutes(-start24HodOsy.Minute);
                    //start24HodOsy = cas.AddMinutes(-cas.Minute).AddSeconds(-cas.Second);
                    
                    osa.Minimum = DateTimeAxis.ToDouble(start24HodOsy);
                    osa.Maximum = DateTimeAxis.ToDouble(start24HodOsy.AddHours(24));
                    posunOsy = -43200;
                    DataPlot.Title = "Průběh teploty (24 hod)";
                    DataPlot.Series[2].IsVisible = true;
                    break;
            }
            DataPlot.InvalidatePlot(true);
            osa.Reset();
        }
    }
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        #region KONSTANTY
        const byte UART_PCK_HEAD = 0x3B;      // 59 ... ;
        const byte UART_PCK_GET_INFO = 0x3F;      // 63 ... ?

        const byte UART_PCK_ROM = 0x61;         // 97  ... a
        const byte UART_PCK_SUPLY = 0x62;       // 98  ... b
        const byte UART_PCK_FULL_SCRPD = 0x63;  // 99  ... c
        const byte UART_PCK_CONFIG = 0x64;      // 100 ... d
        const byte UART_PCK_ERROR = 0x65;       // 101 ... e

        const byte DS18B20_STATE__CIDLO_NENALEZENO = 0;
        const byte DS18B20_STATE__PARAZITNI_NAPAJENI = 1;
        const byte DS18B20_STATE__EXTERNI_NAPAJENI = 2;

        readonly string[] DS18B20_texty = { "Cidlo nenalezeno!", "PARASITE POWER", "EXTERNAL POWER" };

        readonly byte[] dscrc_table = {0, 94,188,226, 97, 63,221,131,194,156,126, 32,163,253, 31, 65,
                                      157,195, 33,127,252,162, 64, 30, 95,  1,227,189, 62, 96,130,220,
                                      35,125,159,193, 66, 28,254,160,225,191, 93,  3,128,222, 60, 98,
                                      190,224,  2, 92,223,129, 99, 61,124, 34,192,158, 29, 67,161,255,
                                      70, 24,250,164, 39,121,155,197,132,218, 56,102,229,187, 89,  7,
                                      219,133,103, 57,186,228,  6, 88, 25, 71,165,251,120, 38,196,154,
                                      101, 59,217,135,  4, 90,184,230,167,249, 27, 69,198,152,122, 36,
                                      248,166, 68, 26,153,199, 37,123, 58,100,134,216, 91,  5,231,185,
                                      140,210, 48,110,237,179, 81, 15, 78, 16,242,172, 47,113,147,205,
                                      17, 79,173,243,112, 46,204,146,211,141,111, 49,178,236, 14, 80,
                                      175,241, 19, 77,206,144,114, 44,109, 51,209,143, 12, 82,176,238,
                                      50,108,142,208, 83, 13,239,177,240,174, 76, 18,145,207, 45,115,
                                      202,148,118, 40,171,245, 23, 73,  8, 86,180,234,105, 55,213,139,
                                      87,  9,235,181, 54,104,138,212,149,203, 41,119,244,170, 72, 22,
                                      233,183, 85, 11,136,214, 52,106, 43,117,151,201, 74, 20,246,168,
                                      116, 42,200,150, 21, 75,169,247,182,232, 10, 84,215,137,107, 53};
        #endregion

        private SerialPort com;
        /* delegat, ktery vola funkci s parametren typu string (pouzit v metode pri prijeti dat) */
        private delegate void UpdateUiTextDelegate(byte []data);
        private static string filename = "teploty.txt";
        StreamWriter sw = new StreamWriter(@filename, true);

        public PlotModel DataPlot { get; set; }
        Graf graf = new Graf();
        DispatcherTimer timerPom = new DispatcherTimer();

        #region konstruktor, onClose metoda

        public MainWindow()
        {
            InitializeComponent();
            /* nacte pouzitelne com porty */
            COM_update();
            com = new SerialPort();//("com3", 9600, Parity.None, 8, 1);

            /* nastavi posledni pouzity COM port pro pripojeni (pokud je dostupny) */
            foreach(ComboBoxItem it in cbPort.Items)
            {
                if (it.Content.ToString() == Properties.Settings.Default.COM_PORT)
                {
                    cbPort.SelectedItem = it;
                }
            }
            
            // pomocny casovac, kdyz nemam pripojeny teplomer
            timerPom.Interval = TimeSpan.FromMilliseconds(10000);
            timerPom.Tick += new EventHandler(timerPom_Tick);
            //timerPom.Start();

           
            Plot1.DataContext = graf;
            graf.nastavRozsahOsyX(Graf.ROZSAHY.PET_MINUT);
            
        }// konec konstruktoru

        double t = 0;
        DateTime dt_proGraf = DateTime.Now;
        void timerPom_Tick(object sender, EventArgs e)
        {
            Random r = new Random();
            graf.pridejBod(DateTime.Now, r.Next(20, 30));
            //dt_proGraf = dt_proGraf.AddMinutes(1);//AddMinutes(30);           
        }

        private void Window_Closing(object sender, CancelEventArgs e)
        {
            com.Close();
            Properties.Settings.Default.COM_PORT = cbPort.SelectionBoxItem.ToString();
            Properties.Settings.Default.Save();
            
            sw.Close();
        }
        
        #endregion

        #region COM connection

        /* Pripojeni k COM */
        private void tbPripoj_Click(object sender, System.Windows.RoutedEventArgs e)
        {
            if (cbPort.SelectedIndex == -1) return;

            if (tbPripoj.IsChecked == false)
            {
                com.Close();
                LED_propoj.Fill = new SolidColorBrush(Colors.Red);
                tbPripoj.Content = "Připojit";
                return;
            }
            COM_set();
            LED_propoj.Fill = new SolidColorBrush(Colors.Yellow);
            chbWait.IsEnabled = false;
            tbPripoj.Content = "Odpojit";

            do
            {                
                // windows zpracuje zpravy, zde vyhazuje obcas vyjimku nullreferenceexception
                Application.Current.Dispatcher.Invoke(DispatcherPriority.Background, new Action(delegate { }));                
                try
                {
                    com.Open();
                    com.DataReceived += new SerialDataReceivedEventHandler(com_DataReceived);
                    byte[] bytePom = new byte[1]; bytePom[0] = UART_PCK_GET_INFO;
                    com.Write(bytePom, 0, 1);
                    LED_propoj.Fill = new SolidColorBrush(Colors.Lime);
                    btnSend.IsEnabled = true;
                }
                catch (Exception ex)
                {
                    if (chbWait.IsChecked == false)
                    {
                        LED_propoj.Fill = new SolidColorBrush(Colors.Red);
                        tbPripoj.IsChecked = false;
                        tbPripoj.Content = "Připojit";
                        MessageBox.Show(ex.Message);
                    }
                }                
            } while (com.IsOpen == false && chbWait.IsChecked == true);

            chbWait.IsEnabled = true;
        }
        /* nastavi com podle volby comboboxu */
        void COM_set()
        {
            com.ReadTimeout = 5000;
            com.PortName = cbPort.SelectionBoxItem.ToString();//"COM3";
            com.BaudRate = Convert.ToInt32(cbRychlost.SelectionBoxItem); //9600;            
            com.DataBits = Convert.ToInt32(cbDatabits.SelectionBoxItem); //8;

            switch (cbParity.SelectedIndex)
            {
                case 0:
                    com.Parity = Parity.None;
                    break;
                case 1:
                    com.Parity = Parity.Odd;
                    break;
                case 2:
                    com.Parity = Parity.Even;
                    break;
            }

            switch (cbStopbits.SelectedIndex)
            {
                case 0:
                    com.StopBits = StopBits.One;
                    break;
                case 1:
                    com.StopBits = StopBits.OnePointFive;
                    break;
                case 2:
                    com.StopBits = StopBits.Two;
                    break;
            }
        }
        /**/
        void COM_update()
        {
            cbPort.Items.Clear();
            foreach (string str in SerialPort.GetPortNames())
            {
                cbPort.Items.Add(new ComboBoxItem() { Content = str });
            }
            cbPort.SelectedIndex = 0;
        }

        #endregion

        #region COM prijata data

        /* udalost pri prijeti dat pres COM */
        bool prijimamPacket = false;
        List<byte> prijataData = new List<byte>(64);
        private void com_DataReceived(object sender, System.IO.Ports.SerialDataReceivedEventArgs e)
        {
            try
            {
                /* precte prichozi data ... zatim nijak neosetreno pri prijeni neceleho packetu !!! */
                //ret = com.ReadLine();

                //int kolikZbyva = com.BytesToRead;
                //byte oneByte = 0;
                while (com.BytesToRead > 0)
                {

                    int comReaded = com.ReadByte();

                    if (comReaded == -1)
                    {
                        MessageBox.Show("Chyba, cteni za koncem datoveho proudu");
                        return;
                    }

                    if (prijimamPacket == false && (byte)comReaded == UART_PCK_HEAD)
                    {
                        prijimamPacket = true;
                        prijataData.Clear();
                    }
                    else if (prijimamPacket == true)
                    {
                        prijataData.Add((byte)comReaded);
                        if (prijataData.Count >= prijataData[0] + 2)
                        {
                            Dispatcher.Invoke(DispatcherPriority.Send, new UpdateUiTextDelegate(DisplayText), prijataData.Skip(1).ToArray());
                            prijimamPacket = false;
                        }
                    }
                }
                /* vyvolani metody, ktera tato data zpracuje */
               // Dispatcher.Invoke(DispatcherPriority.Send, new UpdateUiTextDelegate(DisplayText), ret);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message);
            }
        }
        /* metoda pro zpracovati prijatych dat */
        
        void DisplayText(byte[] data)
        {
            // data na byte pole
            // v prvnim byte je ulozen typ dat , pak velikost dat, ...data... 
            //byte[] bytes = Encoding.Default.GetBytes(ret);


            string strDataHex = "";
            string vyznamDat = "";
            foreach (byte retb in data) 
            {
                strDataHex += String.Format("0x{0:X2} ", retb); // zobrazeni dat v hexadecimalni podobe a jako retezec
            }


            switch (data[0])
            {
                // zobrazeni ROM cidla 
                case UART_PCK_ROM:
                    tblRom.Text = "";
                    if (data.Length > 9)
                    {
                        tblRom.Text = Encoding.Default.GetString(data.Skip(1).ToArray());
                    }
                    else
                    {
                        foreach(byte bytes in data.Skip(1).ToArray())
                        {
                            tblRom.Text += String.Format("0x{0:X2} ", bytes);
                        }
                    }

                    vyznamDat = "ROM čidla";
                    break;

                // zobrazeni zpusobu napajeni cidla 
                case UART_PCK_SUPLY:
                    switch (data[1])
                    {
                        case DS18B20_STATE__CIDLO_NENALEZENO:
                            tblRom.Text = DS18B20_texty[DS18B20_STATE__CIDLO_NENALEZENO];
                            tbNapajeni.Text = tblRom.Text;
                            break;
                        case DS18B20_STATE__EXTERNI_NAPAJENI:
                            tbNapajeni.Text = DS18B20_texty[DS18B20_STATE__EXTERNI_NAPAJENI];
                            break;
                        case DS18B20_STATE__PARAZITNI_NAPAJENI:
                            tbNapajeni.Text = DS18B20_texty[DS18B20_STATE__PARAZITNI_NAPAJENI];
                            break;
                    }
                    vyznamDat = "Způsob napájení čidla";
                    break;

                // zobrazeni obsahu konfiguracnich registru Th, Tl a rozliseni
                case UART_PCK_CONFIG:                   
                    tbRegTh.Text = String.Format("0x{0:X2}   ... Horní mez alarmu  ( {0} °C )", data[1]);
                    tbNastavTh.Text = String.Format("{0}",data[1]);

                    tbRegTl.Text = String.Format("0x{0:X2}   ... Spodní mez alarmu ( {0} °C )", data[2]);
                    tbNastavTl.Text = String.Format("{0}", data[2]);

                    tbRegConf.Text = String.Format("0x{0:X2}", data[3]);
                    switch (data[3])
                    {
                        case 0x7F:
                            tbRegConf.Text += "   ... Rozlišení: 12b. ( 0,0625 °C )";
                            cbNastavRozliseni.SelectedIndex = 0;
                            break;
                        case 0x5F:
                            tbRegConf.Text += "   ... Rozlišení: 11b. ( 0,125 °C )";
                            cbNastavRozliseni.SelectedIndex = 1;
                            break;
                        case 0x3F:
                            tbRegConf.Text += "   ... Rozlišení: 10b. ( 0,25 °C )";
                            cbNastavRozliseni.SelectedIndex = 2;
                            break;
                        case 0x1F:
                            tbRegConf.Text += "   ... Rozlišení: 9b.  ( 0,5 °C )";
                            cbNastavRozliseni.SelectedIndex = 3;
                            break;
                    }
                    vyznamDat = "Nastavení konfiguračního registru";
                    break;

                // zpracovani celeho scratchpad ... zobrazeni teploty a vypocet CRC
                case UART_PCK_FULL_SCRPD:
                    // vypocet CRC (pomoci tabulky hodnot
                    byte crc = 0; byte index = 0;
                    byte[] scratchpad = data.Skip(1).Take(8).ToArray();
                    foreach(byte b in scratchpad)
                    {
                        crc = dscrc_table[(crc ^ scratchpad[index++])];
                    }                                       

                    if (crc == data[9])
                    {
                        // LSB ... bytes[1], MSB ... bytes[2]
                        int teplota_2BYTE = (data[2] << 8) + data[1];
                        bool znamenko = false;
                        double vyslednaTeplota = 0.0;
                        if ((teplota_2BYTE & 0x8000) > 0)
                        {
                            znamenko = true; // teplota je zaporna
                            teplota_2BYTE = (teplota_2BYTE ^ 0xFFFF) + 1; // dvojkovy doplnek
                        }
                        switch (cbNastavRozliseni.SelectedIndex)
                        {
                            case 0:
                                vyslednaTeplota = teplota_2BYTE * 0.0625;
                                break;
                            case 1:
                                vyslednaTeplota = (teplota_2BYTE >> 1) * 0.125;
                                break;
                            case 2:
                                vyslednaTeplota = (teplota_2BYTE >> 2) * 0.25;
                                break;
                            case 3:
                                vyslednaTeplota = (teplota_2BYTE >> 3) * 0.5;
                                break;
                        }

                        tbTeplota.Text = String.Format("{0:F2} °C", vyslednaTeplota);//vysl.ToString() + " °C";
                        zpracujData_graf(vyslednaTeplota);
                        elipseCRC.Fill = new SolidColorBrush(Colors.Lime);

                        vyznamDat = "Konverze teploty, CRC OK";
                    }
                    else
                    {
                        elipseCRC.Fill = new SolidColorBrush(Colors.Red);
                        tbTeplota.Text = "BAD DATA";
                        vyznamDat = "Konverze teploty, CRC FAIL";
                    }
                    break;
                case UART_PCK_ERROR:
                    MessageBox.Show("Chyba MCU ... preteceni bufferu");
                    vyznamDat = "MCU error"; 
                    break;
            }

            TableRow currentRow = new TableRow();
            currentRow.Cells.Add(new TableCell(new Paragraph(new Run(DateTime.Now.ToLongTimeString())))); // cas prijeti
            currentRow.Cells.Add(new TableCell(new Paragraph(new Run(strDataHex)))); // data
            currentRow.Cells.Add(new TableCell(new Paragraph(new Run(vyznamDat)))); // vyznam
            tblRowGrp_zpravy.Rows.Insert(1, currentRow);
        }
        
        #endregion

        #region zalozka graf

        /* zobrazeni dat ve grafu */
        //int counter = 0;
        //int counterHour = 0;
        //int counter12hour = 0;
        private void zpracujData_graf(double aktTeplota)
        {
            graf.pridejBod(DateTime.Now, aktTeplota);
            /* ukladani vsech hodnot*/
            //namerenaData_1h.Add(new KeyValuePair<int, double>(poz, aktTeplota));

            /*
            if (grafData.Count >= 30)
            {
                if (counterHour >= 12*5)
                {
                    namerenaData_12h.Add(new KeyValuePair<int, double>(counter12hour++, namerenaData_1h.Average(x => x.Value)));
                    // uloz do souboru prumer z dat za posledni hodinu + udaj o hodine, ke ktere patri
                    sw.WriteLine("{0} {1:00.0}", DateTime.Now.Hour, namerenaData_1h.Average(x => x.Value));
                    sw.Flush();

                    namerenaData_1h.Clear();
                    counterHour = 0;
                }
                namerenaData_1h.Add(new KeyValuePair<int, double>(counterHour, grafData.Average(x => x.Value)));
                counterHour += 5;
                grafData.Clear();
                counter = 0;

                sw.WriteLine("{0} {1:00.0}", DateTime.Now.Hour, namerenaData_1h.Average(x => x.Value));
                sw.Flush();
            }
            int poz = 0;
            if (grafData.Count > 0)
                poz = grafData.Last().Key + 10;
            grafData.Add(new KeyValuePair<int, double>(poz, aktTeplota));
            counter += 10; 
            */
        }
        /* zmena rozsahu grafu */
        private void ComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {

            if (Plot1 == null) return;

            switch(((ComboBox)sender).SelectedIndex)
            {
                case 0:
                    graf.nastavRozsahOsyX(Graf.ROZSAHY.PET_MINUT);
                    break;
                case 1:
                    graf.nastavRozsahOsyX(Graf.ROZSAHY.HODINA);
                    break;
                case 2:
                    graf.nastavRozsahOsyX(Graf.ROZSAHY.DVACETCTIRY_HODIN);
                    break;
            }

            
        }
        
        #endregion

        #region otevrit nastaveni
        /* tlacitko nastaveni */
        private void Button_Click(object sender, System.Windows.RoutedEventArgs e)
        {
            if(boNastaveni.Visibility == Visibility.Collapsed)
                boNastaveni.Visibility = Visibility.Visible;
            else
                boNastaveni.Visibility = Visibility.Collapsed;
        }
        /* tlacitko refrest com */
        private void Button_Click_1(object sender, RoutedEventArgs e)
        {
            COM_update();
        }
        #endregion

        #region prepinani zalozek

        private void tabControl1_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            switch (((TabControl)sender).SelectedIndex)
            {
                case 0:
                    this.Height = 370.0;
                    break;
                case 1:
                    this.Height = 500.0;
                    break;
                case 2:
                    this.Height = 370.0;
                    break;
                case 3:
                    this.Height = 270.0;
                    break;
            }
            //370
        }
        #endregion

        /* posli nove nastaveni pro cidlo */
        private void btnSend_Click(object sender, RoutedEventArgs e)
        {
            // data packet for mcu
            // ; datalen <data(rozliseni|th|tl), ulozEEPROM> 
            Int16 pom; 
            byte []dataPacket = new byte[5];

            dataPacket[0] = UART_PCK_HEAD;// 59; // ; -  hlavicka packetu

            // prvni dato ... alarm Th
            Int16.TryParse(tbNastavTh.Text, NumberStyles.Number, CultureInfo.CurrentCulture, out pom);
            dataPacket[1] = (byte)pom;

            // druhy dato ... alarm Tl
            Int16.TryParse(tbNastavTl.Text, NumberStyles.Number, CultureInfo.CurrentCulture, out pom);
            dataPacket[2] = (byte)pom;

            // treti dato ... dataPacket[2] = rozliseni
            dataPacket[3] = (byte)Convert.ToInt32(((ComboBoxItem)cbNastavRozliseni.SelectedItem).Tag.ToString(), 16);
            //Int16.TryParse(((ComboBoxItem)cbNastavRozliseni.SelectedItem).Tag.ToString(), NumberStyles.HexNumber, CultureInfo.CurrentCulture, out pom);
            
            // ctvrty dato ... uloz do EEPROM ... 1=uloz, 0=neukladej
            dataPacket[4] = (byte) (((bool)chbUlozDOEEPROM.IsChecked) ? 0x01 : 0x00);

            com.Write(dataPacket, 0, 5);
        }

        private void Button_VymazZpravy_Click(object sender, RoutedEventArgs e)
        {
            if (tblRowGrp_zpravy.Rows.Count > 1)
                tblRowGrp_zpravy.Rows.RemoveRange(1, tblRowGrp_zpravy.Rows.Count - 1);
            //else
            //    MessageBox.Show("a ted by to bylo v prdeli :)");
        }












    }// end class MainWindow
}
