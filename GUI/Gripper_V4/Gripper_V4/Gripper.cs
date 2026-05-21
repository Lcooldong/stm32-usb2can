using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace Gripper_V4
{
    internal class Gripper
    {
        private USB2CAN _serial;
        private CANablePro _can;
        private CancellationTokenSource _cts;

        public delegate void MessageEventHandler(string message);
        public event MessageEventHandler OnMessageReceived;

        // 현재 제작된 그리퍼 범위제한 (중요) - 모터 조립 시 꼭 확인
        public const ushort gripper_close_limit  = 2040;
        public const ushort gripper_open_limit   = 1200;

        public const ushort pusher_push_limit    = 2000;    // 이전 2100
        public const ushort pusher_release_limit = 3800;


        public bool IsConnected { get; private set; }
        public bool IsMonitoring { get; private set; }

        // --- 실시간 상태 데이터 ---
        public ushort CurrentGripperPos { get; private set; }
        public ushort CurrentPusherPos { get; private set; }
        public byte ConnectionStatus { get; private set; } // 0x03이면 정상
        public byte LastErrorCode { get; private set; }


        // 1. 아두이노와 100% 일치하는 8바이트 구조체 설계 (Union 구조 재현)
        [StructLayout(LayoutKind.Explicit, Pack = 1, Size =8)]
        public struct CanPacket
        {
            [FieldOffset(0)] public byte cmd;
            [FieldOffset(1)] public byte id;

            // [송신용] CMD_MOVE 시 사용 (Offset 2부터 시작)
            [FieldOffset(2)] public ushort target_position;
            [FieldOffset(4)] public ushort speed;

            // [수신용] CMD_HEARTBEAT / ARRIVED 수신 시 사용
            [FieldOffset(2)] public ushort gripper_pos;
            [FieldOffset(4)] public ushort pusher_pos;
            [FieldOffset(6)] public byte isConnect;
            [FieldOffset(7)] public byte error_code;
        }

        // 2. 통신 명령어 상수 (아두이노 Enum과 일치)
        public const byte CMD_STOP       = 0x01;
        public const byte CMD_MOVE       = 0x02;
        public const byte CMD_ARRIVED    = 0x03;
        public const byte CMD_GET_STATE  = 0x10;
        public const byte CMD_TORQUE_ON  = 0x11;
        public const byte CMD_TORQUE_OFF = 0x12;
        public const byte CMD_ERROR      = 0xEE;
        public const byte CMD_HEARTBEAT  = 0xFF;

        public const byte ID_GRIPPER = 1;
        public const byte ID_PUSHER = 2;


        
        public Gripper(USB2CAN serial)
        {
            _serial = serial;
            _can = new CANablePro(_serial);
        }

        public Dictionary<string, string> GetUSBDevices() => _serial.GetUSBDevices();

        #region 장치 연결 제어
        public bool Connect(string port, int baudrate = 115200)
        {
            // USB2CAN의 begin 호출
            if (_serial.begin(port, baudrate))
            {
                _can.openChannel(); // CAN 채널 오픈 (O\r)
                _can.read();        // 수신 스레드 시작
                IsConnected = true;

               

                StartMonitoring();
                Console.WriteLine("Gripper CLOSE   :" + gripper_close_limit);
                Console.WriteLine("Gripper OPEN    :" + gripper_open_limit);
                Console.WriteLine("Pusher  PUSH    :" + pusher_push_limit);
                Console.WriteLine("Pusher  RELEASE :" + pusher_release_limit);

                return true;
            }
            return false;
        }

        private void StartMonitoring()
        {
            if (IsMonitoring) return;

            _cts = new CancellationTokenSource();
            IsMonitoring = true;
            Task.Run(() => MonitorLoop(_cts.Token));
        }

        private async Task MonitorLoop(CancellationToken token)
        {
            try
            {
                while (!token.IsCancellationRequested)
                {
                    CanPacket pkt = new CanPacket
                    {
                        cmd = CMD_GET_STATE,
                        id = 0x00
                    };
                    SendPacket(pkt);

                    ParseIncomingData();

                    await Task.Delay(100, token);
                }
            }
            catch (TaskCanceledException)
            {
                // 정상 종료
            }
            catch (Exception ex)
            {
                OnMessageReceived?.Invoke($"[Error] Monitor Loop: {ex.Message}");
            }
            finally
            {
                IsMonitoring = false;
            }
        }


        public void Disconnect()
        {
            _cts?.Cancel();
            _can?.stopRead();
            _can?.closeChannel();
            _serial?.close();
            IsConnected = false;
        }

        // USB2CAN의 자동 연결 기능을 활성화할 때 사용
        public void EnableAutoReconnect()
        {
            _serial.autoConnect();
        }
        #endregion

        #region 명령 송신 (PC -> Arduino)

        private void SendPacket(CanPacket pkt)
        {
            if (!IsConnected) return;
            byte[] data = StructToBytes(pkt);
            // 아두이노가 0x123 필터링 중이므로 ID 0x123으로 송신
            // 8바이트 표준 CAN 프레임 사용
            _can.write(CAN_TYPE.CAN_CLASSIC, 0x123, CAN_DLC.FDCAN_DLC_BYTE_8, data);
        }

        public void MoveGripper(ushort position, ushort speed = 100)
        {
            if (position > gripper_close_limit || position < gripper_open_limit) 
            {
                Console.WriteLine("[Gripper.cs] Please Check Gripper Postion Value");
                return;
            } 

            Console.WriteLine($"{position} {speed}");
            CanPacket pkt = new CanPacket
            {
                cmd = CMD_MOVE,
                id = ID_GRIPPER,
                target_position = position,
                speed = speed
            };
            SendPacket(pkt);
        }

        public void MovePusher(ushort position, ushort speed = 100)
        {
            if (position > pusher_release_limit || position < pusher_push_limit)
            {
                Console.WriteLine("[Gripper.cs] Please Check Pusher Postion Value");
                return;
            }

            CanPacket pkt = new CanPacket
            {
                cmd = CMD_MOVE,
                id = ID_PUSHER,
                target_position = position,
                speed = speed
            };
            SendPacket(pkt);
        }

        public void EmergencyStop()
        {
            CanPacket pkt = new CanPacket { cmd = CMD_STOP };
            SendPacket(pkt);
        }

        public void SetTorque(byte motorId, bool isOn)
        {
            CanPacket pkt = new CanPacket
            {
                cmd = isOn ? CMD_TORQUE_ON : CMD_TORQUE_OFF,
                id = motorId
            };
            SendPacket(pkt);
        }

        public void RequestState()
        {
            CanPacket pkt = new CanPacket { cmd = CMD_GET_STATE };
            SendPacket(pkt);
        }
        #endregion

        #region 데이터 수신 및 파싱 (Arduino -> PC)

        /// <summary>
        /// 이 메서드를 타이머(예: 20ms)에서 주기적으로 호출하여 상태를 갱신하십시오.
        /// </summary>
        public void ParseIncomingData()
        {
            // 현재 큐에 쌓인 개수를 먼저 파악 (동기화 이슈 방지)
            int currentCount = _can.packetQueue.Count;

            if (currentCount == 0) return;

            // 현재 쌓여 있는 만큼만 루프를 돌며 처리
            for (int i = 0; i < currentCount; i++)
            {
                // DequeueSafe()가 null을 리턴하면 루프 종료
                var pktData = _can.packetQueue.DequeueSafe();
                if (pktData == null) continue;

                try
                {
                    byte[] rawBytes = pktData.data.ToArray();
                    string rawHex = string.Join(" ", rawBytes.Select(b => b.ToString("X2")));

                    if (rawBytes.Length < Marshal.SizeOf<CanPacket>())
                    {
                        OnMessageReceived?.Invoke($"[RX Invalid] DLC too short: {rawBytes.Length}");
                        continue;
                    }

                    CanPacket received = BytesToStruct(rawBytes);

                    string fullPacketLog = $"[RX] {rawHex} | CMD:0x{received.cmd:X2} ID:{received.id} " +
                               $"G_POS:{received.gripper_pos} P_POS:{received.pusher_pos} " +
                               $"CONN:0x{received.isConnect:X2} ERR:0x{received.error_code:X2}";

                    switch (received.cmd)
                    {
                        case CMD_HEARTBEAT:
                            CurrentGripperPos = received.gripper_pos;
                            CurrentPusherPos = received.pusher_pos;
                            ConnectionStatus = received.isConnect;
                            LastErrorCode = received.error_code;

                            OnMessageReceived?.Invoke(fullPacketLog);
                            break;

                        case CMD_ARRIVED:
                            string motor = (received.id == ID_GRIPPER) ? "GRIPPER" : "PUSHER";
                            ushort pos = (received.id == ID_GRIPPER)
                                ? received.gripper_pos
                                : received.pusher_pos;

                            OnMessageReceived?.Invoke($"[ARRIVED] {motor} reached {pos}");
                            break;

                        case CMD_STOP:
                            OnMessageReceived?.Invoke("[STOP] Emergency Stop Ack");
                            break;
                        case CMD_ERROR:

                            break;
                        default:
                            // 정의되지 않은 CMD라도 원본 패킷은 보여줌
                            OnMessageReceived?.Invoke($"[UNKNOWN] {fullPacketLog}");
                            break;
                    }
                }
                catch (Exception ex)
                {
                    // 데이터 변환 오류 시 로그만 찍고 다음 패킷으로
                    System.Diagnostics.Debug.WriteLine("Parse Error: " + ex.Message);
                }
            }
        }
        #endregion

        #region Marshal Helper (구조체 <-> 바이트배열 변환)
        private byte[] StructToBytes(CanPacket pkt)
        {
            int size = Marshal.SizeOf(pkt);
            byte[] arr = new byte[size];
            IntPtr ptr = Marshal.AllocHGlobal(size);
            Marshal.StructureToPtr(pkt, ptr, false);
            Marshal.Copy(ptr, arr, 0, size);
            Marshal.FreeHGlobal(ptr);
            return arr;
        }

        private CanPacket BytesToStruct(byte[] data)
        {
            GCHandle handle = GCHandle.Alloc(data, GCHandleType.Pinned);
            try
            {
                return (CanPacket)Marshal.PtrToStructure(handle.AddrOfPinnedObject(), typeof(CanPacket));
            }
            finally
            {
                handle.Free();
            }
        }
        #endregion



    }
}