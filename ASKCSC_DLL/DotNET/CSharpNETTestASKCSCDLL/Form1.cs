using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace CSharpNETTestASKCSCDLL
{
    public partial class Form1 : Form
    {
        public Form1()
        {
            InitializeComponent();
        }

        private void Form1_Load(object sender, EventArgs e)
        {

        }

        private void Button1_Click(object sender, EventArgs e)
        {
            AskReaderLib.CSC.sCARD_SearchExtTag SearchExtender;
            int Status;
            byte[] ATR;
            ATR = new byte[200];
            int lgATR;
            lgATR = 200;
            int Com=0;
            int SearchMask;

            txtCom.Text = "";
            txtCard.Text = "";

            try
            {
                AskReaderLib.CSC.SearchCSC();
                // user can also use line below to speed up coupler connection
                //AskReaderLib.CSC.Open ("COM2");

                // Define type of card to be detected: number of occurence for each loop
                SearchExtender.CONT = 0;
                SearchExtender.ISOB = 2;
                SearchExtender.ISOA = 2;
                SearchExtender.TICK = 1;
                SearchExtender.INNO = 2;
                SearchExtender.MIFARE = 0;
                SearchExtender.MV4k = 0;
                SearchExtender.MV5k = 0;
                SearchExtender.MONO = 0;

                // Define type of card to be detected
                SearchMask = AskReaderLib.CSC.SEARCH_MASK_INNO | AskReaderLib.CSC.SEARCH_MASK_ISOB | AskReaderLib.CSC.SEARCH_MASK_ISOA | AskReaderLib.CSC.SEARCH_MASK_TICK;
                Status = AskReaderLib.CSC.SearchCardExt(ref SearchExtender, SearchMask, 1, 20, ref Com, ref lgATR, ATR);

                if (Status != AskReaderLib.CSC.RCSC_Ok)
                    txtCom.Text =  "Error :" + Status.ToString ("X");
                else
                    txtCom.Text = Com.ToString("X");

                if (Com == 2)
                    txtCard.Text = "ISO14443A-4 no Calypso";
                else if (Com == 3)
                    txtCard.Text = "INNOVATRON";
                else if (Com == 4)
                    txtCard.Text = "ISOB14443B-4 Calypso";
                else if (Com == 5)
                    txtCard.Text = "Mifare";
                else if (Com == 6)
                    txtCard.Text = "CTS or CTM";
                else if (Com == 8)
                    txtCard.Text = "ISO14443A-3 ";
                else if (Com == 9)
                    txtCard.Text = "ISOB14443B-4 Calypso";
                else if (Com == 12)
                    txtCard.Text = "ISO14443A-4 Calypso";
                else if (Com == 0x6F)
                    txtCard.Text = "Card not found";
                else
                    txtCard.Text = "";
            }
            catch
            {
                MessageBox.Show("Error on trying do deal with reader");
            }
            AskReaderLib.CSC.Close();
        }
    }
}