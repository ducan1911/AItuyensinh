-- ============================================================
-- SCHEMA CHO KIOSK AI TƯ VẤN TUYỂN SINH
-- Dán toàn bộ nội dung này vào Supabase Dashboard → SQL Editor → Run
-- ============================================================

-- 1) Bảng lưu câu hỏi + câu trả lời + phân loại ngành
create table if not exists public.questions (
  id          uuid primary key default gen_random_uuid(),
  created_at  timestamptz not null default now(),
  question    text not null,               -- câu hỏi học sinh (do STT chuyển ra)
  answer      text,                        -- câu trả lời của AI
  category    text not null default 'Khác',-- CNTT / Kinh tế / Ngôn ngữ / Học phí - Học bổng / Khác...
  device_id   text                         -- để phân biệt nếu sau này có >1 kiosk
);

-- 2) Bật Row Level Security (RLS) — bắt buộc để bảo vệ dữ liệu công khai
alter table public.questions enable row level security;

-- 3) CHỈ cho phép đọc công khai (dashboard hiển thị TV dùng anon key)
--    Không tạo policy INSERT cho anon/public => ai mở dashboard trên trình duyệt
--    cũng KHÔNG thể tự chèn dữ liệu giả vào bảng.
create policy "Cho phép đọc công khai"
  on public.questions
  for select
  to anon
  using (true);

-- 4) Việc GHI dữ liệu chỉ thực hiện từ phía server (node HTTP Request trong Dify,
--    hoặc script backend) bằng SERVICE ROLE KEY — key này giữ bí mật, KHÔNG bao
--    giờ đặt trong dashboard.html hay bất kỳ code chạy trên trình duyệt.
--    Service role tự động bypass RLS nên không cần thêm policy insert ở đây.

-- 5) Bật Realtime cho bảng này để dashboard nhận cập nhật tức thời khi có câu hỏi mới
--    (Nếu lệnh dưới báo lỗi "already a member", nghĩa là đã bật sẵn, bỏ qua được.)
--    Cách khác: vào Supabase Dashboard → Table Editor → questions → bật toggle "Realtime".
alter publication supabase_realtime add table public.questions;

-- 6) (Tuỳ chọn) Index để dashboard truy vấn thống kê theo category nhanh hơn
create index if not exists idx_questions_category on public.questions (category);
create index if not exists idx_questions_created_at on public.questions (created_at desc);
