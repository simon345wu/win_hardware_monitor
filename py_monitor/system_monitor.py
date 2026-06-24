import threading
import time
import winreg
import psutil
import numpy as np
from collections import deque

class CircularBuffer:
    def __init__(self, max_size=300):
        self.max_size = max_size
        self.data = deque(maxlen=max_size)

    def resize(self, new_size):
        self.max_size = new_size
        # 建立一個新的 deque 並複製舊資料（若有需要截斷或保留）
        old_data = list(self.data)
        self.data = deque(old_data, maxlen=new_size)

    def add(self, val):
        self.data.append(val)

    def get_contiguous(self):
        # 轉換為 list，適合 plot 繪圖
        return list(self.data)

    def get_latest(self):
        if not self.data:
            return 0.0
        return self.data[-1]

class MonitorSnapshot:
    def __init__(self):
        self.core_histories = []       # List of lists
        self.avg_cpu_history = []      # List
        self.memory_history = []       # List
        self.current_core_usages = []  # List
        self.current_avg_cpu = 0.0
        self.current_memory_pct = 0.0
        self.total_memory_gb = 0.0
        self.used_memory_gb = 0.0
        self.avail_memory_gb = 0.0
        self.sample_count = 0

class SystemMonitor:
    def __init__(self):
        self._interval_ms = 200
        self._history_length = 300
        self._running = False
        self._lock = threading.Lock()
        self._worker_thread = None

        # 1. 取得 CPU 型號
        self._cpu_model_name = self._query_cpu_model_name()

        # 2. 取得核心數
        self._core_count = psutil.cpu_count(logical=True)
        if self._core_count is None:
            self._core_count = 1

        # 3. 初始化數據緩衝區
        self._core_histories = [CircularBuffer(self._history_length) for _ in range(self._core_count)]
        self._avg_cpu_history = CircularBuffer(self._history_length)
        self._memory_history = CircularBuffer(self._history_length)

        # 為了初始化 psutil CPU 採樣器（第一次調用會回傳 0）
        psutil.cpu_percent(percpu=True)
        time.sleep(0.05)
        psutil.cpu_percent(percpu=True)

        # 記憶體靜態/初始資訊
        self._total_memory_gb = 0.0
        self._used_memory_gb = 0.0
        self._avail_memory_gb = 0.0

    def start(self):
        if self._running:
            return
        self._running = True
        self._collect_metrics()  # 建立基準數據
        self._worker_thread = threading.Thread(target=self._worker_loop, daemon=True)
        self._worker_thread.start()

    def stop(self):
        self._running = False
        if self._worker_thread and self._worker_thread.is_alive():
            self._worker_thread.join(timeout=1.0)

    def set_interval_ms(self, ms):
        if ms < 50:
            ms = 50
        with self._lock:
            self._interval_ms = ms

    def get_interval_ms(self):
        with self._lock:
            return self._interval_ms

    def set_history_length(self, length):
        if length < 10:
            length = 10
        if length > 1000:
            length = 1000
        
        with self._lock:
            if length == self._history_length:
                return
            self._history_length = length
            for buf in self._core_histories:
                buf.resize(length)
            self._avg_cpu_history.resize(length)
            self._memory_history.resize(length)

    def get_history_length(self):
        with self._lock:
            return self._history_length

    def get_core_count(self):
        return self._core_count

    def get_cpu_model_name(self):
        return self._cpu_model_name

    def get_snapshot(self, snapshot: MonitorSnapshot):
        with self._lock:
            snapshot.core_histories = [buf.get_contiguous() for buf in self._core_histories]
            snapshot.current_core_usages = [buf.get_latest() for buf in self._core_histories]
            
            snapshot.avg_cpu_history = self._avg_cpu_history.get_contiguous()
            snapshot.memory_history = self._memory_history.get_contiguous()
            
            snapshot.current_avg_cpu = self._avg_cpu_history.get_latest()
            snapshot.current_memory_pct = self._memory_history.get_latest()
            
            snapshot.total_memory_gb = self._total_memory_gb
            snapshot.used_memory_gb = self._used_memory_gb
            snapshot.avail_memory_gb = self._avail_memory_gb
            snapshot.sample_count = len(snapshot.avg_cpu_history)

    def _worker_loop(self):
        while self._running:
            self._collect_metrics()
            
            # 回應式 sleep：每次睡 10ms，以便快速退出
            sleep_remaining = self.get_interval_ms()
            while sleep_remaining > 0 and self._running:
                sleep_time = 10 if sleep_remaining > 10 else sleep_remaining
                time.sleep(sleep_time / 1000.0)
                sleep_remaining -= sleep_time

    def _collect_metrics(self):
        # 1. 收集記憶體數據
        mem = psutil.virtual_memory()
        total_gb = mem.total / (1024.0 ** 3)
        avail_gb = mem.available / (1024.0 ** 3)
        used_gb = total_gb - avail_gb
        mem_pct = mem.percent

        # 2. 收集 CPU 數據
        # psutil.cpu_percent(percpu=True) 取得的是與上一次呼叫之間的 CPU 使用率
        core_percentages = psutil.cpu_percent(percpu=True)
        if not core_percentages or len(core_percentages) != self._core_count:
            # 安全防護
            core_percentages = [0.0] * self._core_count

        avg_cpu = sum(core_percentages) / self._core_count

        # 3. 寫入緩衝區 (線程安全)
        with self._lock:
            self._total_memory_gb = total_gb
            self._used_memory_gb = used_gb
            self._avail_memory_gb = avail_gb
            
            for i in range(self._core_count):
                self._core_histories[i].add(core_percentages[i])
            self._avg_cpu_history.add(avg_cpu)
            self._memory_history.add(mem_pct)

    def _query_cpu_model_name(self):
        try:
            key = winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, r"HARDWARE\DESCRIPTION\System\CentralProcessor\0")
            val, _ = winreg.QueryValueEx(key, "ProcessorNameString")
            winreg.CloseKey(key)
            return val.strip()
        except Exception:
            return "Windows Processor"
