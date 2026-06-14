# gPBL2026
# 🚪 Hệ Thống Cửa Bảo Mật Thông Minh với Xác Thực Nhịp Gõ Cửa

![Project Status](https://img.shields.io/badge/Status-Completed-success)
![Program](https://img.shields.io/badge/Program-gPBL_2026_NetSoft-blue)
![Platform](https://img.shields.io/badge/Platform-Arduino_%7C_Raspberry_Pi-red)

## 💡 Tổng Quan Dự Án
Dự án này giới thiệu hệ thống bảo mật IoT đa lớp. Hệ thống thay thế chìa khóa truyền thống bằng ổ khóa xác thực dựa trên nhịp gõ cửa (Rhythm lock), kết hợp với khả năng giám sát và điều khiển từ xa thông qua Telegram.

**Nhóm 10:** **Võ Quang Bách**, Ema Nakagawa, Shingo Hirachi, Châu Quốc Tuấn, Nguyễn Lê Anh Tài.

## 🎯 Mục Tiêu
*   Phát triển các kỹ năng thực tế về tích hợp hệ thống IoT.
*   Cải thiện kỹ năng làm việc nhóm và hợp tác dự án đa văn hóa.
*   Ứng dụng hệ thống nhúng và công nghệ cảm biến vào các bài toán thực tế.
*   Nâng cao khả năng giải quyết vấn đề và thiết kế hệ thống.
*   Xây dựng một giải pháp bảo mật đáng tin cậy, thân thiện với người dùng và mang tính đổi mới.

## ⚙️ Kiến Trúc Hệ Thống & Quy Trình Hoạt Động
Hệ thống hoạt động dựa trên kiến trúc Master-Slave sử dụng Raspberry Pi và Arduino, giao tiếp với nhau qua cổng Serial (USB).

1.  **Phát hiện & Kích hoạt:** **Cảm biến chuyển động (Motion Sensor)** phát hiện người tiến đến và đánh thức hệ thống.
2.  **Tương tác bước 1 (Xác thực hình ảnh):** Khi khách chạm vào **Cảm biến chạm (Touch Sensor)**, **Camera Raspberry Pi (5MP)** ngay lập tức chụp lại hình ảnh của người đó.
3.  **Thông báo đám mây:** Raspberry Pi xử lý hình ảnh và tải lên máy chủ đám mây của Telegram. Chủ nhà sẽ nhận được thông báo tức thì kèm hình ảnh thông qua **Telegram Bot** trên điện thoại/laptop.
4.  **Tương tác bước 2 (Xác thực vật lý):** Hệ thống chờ người dùng nhập mật khẩu thông qua **Cảm biến rung (Vibration Sensor)** (Xác thực bằng nhịp gõ cửa / Mật khẩu nhịp điệu).
5.  **Kiểm soát truy cập:** 
    *   *Tại chỗ:* Nếu nhịp gõ khớp với nhịp điệu đã được lập trình sẵn, **Động cơ Servo (Servo Motor)** sẽ mở cửa.
    *   *Từ xa:* Chủ nhà có thể bỏ qua bước gõ cửa và gửi lệnh "open" (mở) trực tiếp thông qua Telegram Bot để cho khách vào.
    *   *Cảnh báo:* **Còi chip (Buzzer)** sẽ phát ra âm thanh báo động nếu có hành vi cố tình xâm nhập không hợp lệ.

## 🛠️ Linh Kiện Phần Cứng
*   **Vi điều khiển:** Arduino
*   **Máy tính nhúng:** Raspberry Pi
*   **Camera:** Module Camera Raspberry Pi 5 Megapixel
*   **Cơ cấu chấp hành:** Động cơ Servo
*   **Cảm biến:**
    *   Cảm biến rung
    *   Cảm biến chuyển động
    *   Cảm biến chạm
*   **Đầu ra âm thanh:** Còi chip Buzzer

## 💻 Nhiệm Vụ Kỹ Thuật & Luồng Dữ Liệu
*   **Arduino:** Được lập trình bằng C++ để liên tục đọc và lọc tín hiệu từ các cảm biến chuyển động, chạm và rung.
*   **Giao tiếp Serial:** Arduino truyền luồng dữ liệu cảm biến đã được cấu trúc tới Raspberry Pi thông qua cáp USB.
*   **Raspberry Pi:** Đóng vai trò là trung tâm điều khiển. Nó phân tích dữ liệu từ Arduino, kích hoạt Camera Pi và giao tiếp với API của Telegram.
*   **Tích hợp Telegram:** Giao tiếp hai chiều. Gửi cảnh báo theo thời gian thực (hình ảnh/văn bản) tới người dùng, đồng thời liên tục nhận các lệnh webhook từ người dùng để điều khiển khóa cửa vật lý.
