import av
import cv2

class Camera:
  def __init__(self, cam_type_state, stream_type, camera_id):
    try:
      camera_id = int(camera_id)
    except ValueError: # allow strings, ex: /dev/video0
      pass
    self.cam_type_state = cam_type_state
    self.stream_type = stream_type
    self.cur_frame_id = 0

    self.container = av.open(camera_id)
    assert self.container.streams.video, f"Can't open video stream for camera {camera_id}"
    self.video_stream = self.container.streams.video[0]
    self.W = self.video_stream.codec_context.width
    self.H = self.video_stream.codec_context.height

  @classmethod
  def bgr2nv12(self, bgr):
    frame = av.VideoFrame.from_ndarray(bgr, format='bgr24')
    return frame.reformat(format='nv12').to_ndarray()

  def read_frames(self):
    for frame in self.container.decode(self.video_stream):
      img = frame.to_rgb().to_ndarray()[:,:, ::-1] # convert to bgr24
      yuv = Camera.bgr2nv12(img)
      yield yuv.data.tobytes()
    self.container.close()

class CameraMJPG:
    def __init__(self, cam_type_state, stream_type, camera_id):
        try:
            camera_id = int(camera_id)
        except ValueError:
            pass

        self.cap = cv2.VideoCapture(camera_id)
        if not self.cap.isOpened():
            raise IOError(f"无法打开摄像头设备 {camera_id}")

        # 优先尝试 UYVY 格式（rkisp 输出格式），失败时回退 MJPG/YUYV/YUY2
        self._configure_camera_format()
        actual_format = self._get_current_format()
        print("数据格式: ", actual_format)
        # 若 UYVY 不支持，回退默认

        # 获取实际设置的FPS并打印
        self.fps = self.cap.get(cv2.CAP_PROP_FPS)
        print(f"摄像头初始化后的FPS设置: {self.fps}")


        # 获取分辨率
        self.W = int(self.cap.get(cv2.CAP_PROP_FRAME_WIDTH))
        self.H = int(self.cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
        self.cur_frame_id = 0
        self.cam_type_state = cam_type_state
        self.stream_type = stream_type
        self.current_format = actual_format  # 记录当前格式用于后续处理
        print(f"摄像头实际分辨率: {self.W}x{self.H}, 像素格式: {actual_format}")

    def _configure_camera_format(self):
        """尝试设置摄像头支持的像素格式，优先 UYVY（rkisp 输出格式），回退 MJPG/YUYV/YUY2"""
        for fmt in ("UYVY", "MJPG", "YUYV", "YUY2"):
            fourcc = cv2.VideoWriter_fourcc(*fmt)
            self.cap.set(cv2.CAP_PROP_FOURCC, fourcc)
            self.cap.set(cv2.CAP_PROP_FOURCC, fourcc)
            self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1280)  # 优先选择最高分辨率
            self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 720)
            self.cap.set(cv2.CAP_PROP_FPS, 20)
            if self._get_current_format() == fmt:
                break


    def _get_current_format(self):
        """获取当前实际格式"""
        fourcc_code = int(self.cap.get(cv2.CAP_PROP_FOURCC))
        return ''.join([chr((fourcc_code >> 8 * i) & 0xFF) for i in range(4)])

    @staticmethod
    def _bgr_to_nv12(bgr_frame):
        frame = av.VideoFrame.from_ndarray(bgr_frame, format='bgr24')
        return frame.reformat(format='nv12').to_ndarray().data.tobytes()

    def read_frames(self):
        """持续读取帧并转换为 NV12"""
        warned_raw = False
        while True:
            ret, frame = self.cap.read()
            if not ret:
                break

            # OpenCV V4L2 后端对 MJPG/YUYV 会自动转换为 BGR；若返回非 3 通道（如 RAW），尝试转换
            if frame.ndim == 3 and frame.shape[2] == 3:
                yield self._bgr_to_nv12(frame)
            else:
                if not warned_raw:
                    print(f"警告: 摄像头返回非 RGB 帧，形状 {frame.shape}，尝试转换")
                    warned_raw = True
                if frame.ndim == 2:
                    bgr = cv2.cvtColor(frame, cv2.COLOR_GRAY2BGR)
                    yield self._bgr_to_nv12(bgr)
                elif frame.ndim == 3 and frame.shape[2] == 2:
                    bgr = cv2.cvtColor(frame, cv2.COLOR_YUV2BGR_YUYV)
                    yield self._bgr_to_nv12(bgr)
                else:
                    raise ValueError(f"无法处理摄像头帧格式: {frame.shape}")
        self.cap.release()

    def __del__(self):
        if hasattr(self, 'cap') and self.cap.isOpened():
            self.cap.release()