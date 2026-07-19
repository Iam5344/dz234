using System;
using System.Collections.Generic;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Windows.Forms;

namespace WinFormsUDPChat
{
    public partial class MainForm : Form
    {
        private class ChatContact
        {
            public string Name { get; set; } = null!;
            public IPAddress IpAddress { get; set; } = null!;
            public int RemotePort { get; set; }
            public string History { get; set; } = "";

            public override string ToString() => $"{Name} ({IpAddress}:{RemotePort})";
        }

        private UdpClient? _udpListener;
        private Thread? _receiveThread;
        private bool _isListening;
        private int _localPort;
        private List<ChatContact> _contacts = new List<ChatContact>();
        private ChatContact? _activeChat;

        private ListBox lstContacts = null!;
        private TextBox txtLog = null!;
        private TextBox txtMessage = null!;
        private Button btnSend = null!;
        private TextBox txtLocalPort = null!;
        private TextBox txtContactName = null!;
        private TextBox txtRemoteIP = null!;
        private TextBox txtRemotePort = null!;
        private Button btnAddContact = null!;
        private Button btnStartListen = null!;

        public MainForm()
        {
            InitializeComponent();
        }

        private void InitializeComponent()
        {
            this.Size = new System.Drawing.Size(750, 480);
            this.Text = "UDP Chat with History";

            Panel topPanel = new Panel { Dock = DockStyle.Top, Height = 50 };
            Label lblLocalPort = new Label { Text = "Local Port:", Left = 10, Top = 15, Width = 70 };
            txtLocalPort = new TextBox { Text = "8001", Left = 80, Top = 12, Width = 60 };
            btnStartListen = new Button { Text = "Start Listen", Left = 150, Top = 10, Width = 100 };
            btnStartListen.Click += BtnStartListen_Click;

            topPanel.Controls.AddRange(new Control[] { lblLocalPort, txtLocalPort, btnStartListen });

            Panel leftPanel = new Panel { Dock = DockStyle.Left, Width = 220 };
            
            Label lblAdd = new Label { Text = "Add Contact", Left = 10, Top = 10, Font = new System.Drawing.Font(FontFamily.GenericSansSerif, 9, System.Drawing.FontStyle.Bold) };
            txtContactName = new TextBox { PlaceholderText = "Name", Left = 10, Top = 35, Width = 200 };
            txtRemoteIP = new TextBox { PlaceholderText = "Remote IP (127.0.0.1)", Left = 10, Top = 65, Width = 200 };
            txtRemotePort = new TextBox { PlaceholderText = "Remote Port", Left = 10, Top = 95, Width = 200 };
            
            btnAddContact = new Button { Text = "Add Contact", Left = 10, Top = 125, Width = 200 };
            btnAddContact.Click += BtnAddContact_Click;

            Label lblChats = new Label { Text = "Active Chats:", Left = 10, Top = 160 };
            lstContacts = new ListBox { Left = 10, Top = 185, Width = 200, Height = 200 };
            lstContacts.SelectedIndexChanged += LstContacts_SelectedIndexChanged;

            leftPanel.Controls.AddRange(new Control[] { lblAdd, txtContactName, txtRemoteIP, txtRemotePort, btnAddContact, lblChats, lstContacts });

            Panel mainPanel = new Panel { Dock = DockStyle.Fill };
            txtLog = new TextBox { MultiLine = true, ReadOnly = true, ScrollBars = ScrollBars.Vertical, Dock = DockStyle.Fill };
            
            Panel bottomPanel = new Panel { Dock = DockStyle.Bottom, Height = 50 };
            txtMessage = new TextBox { Left = 10, Top = 15, Width = 400 };
            txtMessage.KeyDown += TxtMessage_KeyDown;
            btnSend = new Button { Text = "Send", Left = 420, Top = 12, Width = 80 };
            btnSend.Click += BtnSend_Click;
            bottomPanel.Controls.AddRange(new Control[] { txtMessage, btnSend });

            mainPanel.Controls.Add(txtLog);
            mainPanel.Controls.Add(bottomPanel);

            this.Controls.Add(mainPanel);
            this.Controls.Add(leftPanel);
            this.Controls.Add(topPanel);
            
            this.FormClosing += MainForm_FormClosing;
        }

        private void BtnStartListen_Click(object? sender, EventArgs e)
        {
            if (_isListening) return;

            if (!int.TryParse(txtLocalPort.Text, out _localPort))
            {
                MessageBox.Show("Invalid local port");
                return;
            }

            try
            {
                _udpListener = new UdpClient(_localPort);
                _isListening = true;
                txtLocalPort.Enabled = false;
                btnStartListen.Enabled = false;

                _receiveThread = new Thread(ReceiveData);
                _receiveThread.IsBackground = true;
                _receiveThread.Start();
            }
            catch (Exception ex)
            {
                MessageBox.Show("Error starting listener: " + ex.Message);
            }
        }

        private void BtnAddContact_Click(object? sender, EventArgs e)
        {
            if (string.IsNullOrWhiteSpace(txtContactName.Text) || 
                !IPAddress.TryParse(txtRemoteIP.Text, out IPAddress? remoteIP) || 
                !int.TryParse(txtRemotePort.Text, out int remotePort))
            {
                MessageBox.Show("Please fill all contact fields correctly.");
                return;
            }

            ChatContact contact = new ChatContact
            {
                Name = txtContactName.Text.Trim(),
                IpAddress = remoteIP,
                RemotePort = remotePort
            };

            _contacts.Add(contact);
            lstContacts.Items.Add(contact);

            txtContactName.Clear();
            txtRemoteIP.Clear();
            txtRemotePort.Clear();
        }

        private void LstContacts_SelectedIndexChanged(object? sender, EventArgs e)
        {
            if (lstContacts.SelectedItem is ChatContact selectedContact)
            {
                if (_activeChat != null)
                {
                    _activeChat.History = txtLog.Text;
                }

                _activeChat = selectedContact;
                txtLog.Text = _activeChat.History;
            }
        }

        private void BtnSend_Click(object? sender, EventArgs e)
        {
            SendMessage();
        }

        private void TxtMessage_KeyDown(object? sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Enter)
            {
                e.SuppressKeyPress = true;
                SendMessage();
            }
        }

        private void SendMessage()
        {
            if (_activeChat == null)
            {
                MessageBox.Show("Select a chat from the list first.");
                return;
            }

            string text = txtMessage.Text.Trim();
            if (string.IsNullOrEmpty(text)) return;

            UdpClient client = new UdpClient();
            IPEndPoint endPoint = new IPEndPoint(_activeChat.IpAddress, _activeChat.RemotePort);

            try
            {
                byte[] bytes = Encoding.Unicode.GetBytes(text);
                client.Send(bytes, bytes.Length, endPoint);

                string formattedMessage = $"[Me]: {text}{Environment.NewLine}";
                txtLog.AppendText(formattedMessage);
                _activeChat.History = txtLog.Text;

                txtMessage.Clear();
            }
            catch (Exception ex)
            {
                MessageBox.Show("Send error: " + ex.Message);
            }
            finally
            {
                client.Close();
            }
        }

        private void ReceiveData()
        {
            try
            {
                while (_isListening && _udpListener != null)
                {
                    IPEndPoint ipEnd = null!;
                    byte[] response = _udpListener.Receive(ref ipEnd);
                    string message = Encoding.Unicode.GetString(response);

                    this.Invoke((MethodInvoker)delegate
                    {
                        ChatContact? senderContact = _contacts.Find(c => 
                            c.IpAddress.Equals(ipEnd.Address) && c.RemotePort == ipEnd.Port);

                        if (senderContact == null)
                        {
                            senderContact = _contacts.Find(c => c.IpAddress.Equals(ipEnd.Address));
                        }

                        if (senderContact == null)
                        {
                            senderContact = new ChatContact
                            {
                                Name = $"Unknown ({ipEnd.Address})",
                                IpAddress = ipEnd.Address,
                                RemotePort = ipEnd.Port
                            };
                            _contacts.Add(senderContact);
                            lstContacts.Items.Add(senderContact);
                        }

                        string formattedMessage = $"[{senderContact.Name}]: {message}{Environment.NewLine}";

                        if (_activeChat == senderContact)
                        {
                            txtLog.AppendText(formattedMessage);
                            _activeChat.History = txtLog.Text;
                        }
                        else
                        {
                            senderContact.History += formattedMessage;
                        }
                    });
                }
            }
            catch (SocketException)
            {
            }
            catch (Exception ex)
            {
                this.Invoke((MethodInvoker)delegate
                {
                    MessageBox.Show("Receive error: " + ex.Message);
                });
            }
        }

        private void MainForm_FormClosing(object? sender, FormClosingEventArgs e)
        {
            _isListening = false;
            if (_udpListener != null)
            {
                _udpListener.Close();
            }
        }
    }
}
