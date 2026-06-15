import { useState, useEffect, useCallback, useRef } from "react";
import { motion, AnimatePresence } from "framer-motion";
import steeringWheel from "./assets/steering-wheel.png";
import {
  Car, Camera, Sliders, Cpu, ParkingSquare, Bell, Settings,
  Wifi, Battery, AlertTriangle, ChevronUp, ChevronDown, ChevronLeft,
  ChevronRight, Square, Gauge, Thermometer, MapPin, Navigation,
  Shield, Eye, Lock, Zap, PlayCircle, PauseCircle, XCircle,
  LayoutDashboard, Volume2, Sun, Moon, ToggleLeft, ToggleRight,
  Radio, Crosshair, Activity, Circle
} from "lucide-react";

// Mock data
const MOCK_GPS = { lat: 33.5731, lng: -7.5898 };

function useSensors() {
  const [sensors, setSensors] = useState({ front: 1.6, rear: 2.1, left: 1.2, right: 0.3 });
  useEffect(() => {
    const id = setInterval(() => {
      setSensors(prev => ({
        front: Math.max(0.1, Math.min(4, prev.front + (Math.random() - 0.5) * 0.15)),
        rear:  Math.max(0.1, Math.min(4, prev.rear  + (Math.random() - 0.5) * 0.12)),
        left:  Math.max(0.1, Math.min(4, prev.left  + (Math.random() - 0.5) * 0.1)),
        right: Math.max(0.05, Math.min(1.0,
          Math.random() < 0.25
            ? 0.1 + Math.random() * 0.35
            : prev.right + (Math.random() - 0.5) * 0.1)),
      }));
    }, 1200);
    return () => clearInterval(id);
  }, []);
  return sensors;
}

function useTelemetry() {
  const [tel, setTel] = useState({ battery: 78, signal: 92, motorTemp: 42, speed: 0, gear: "P" });
  useEffect(() => {
    const id = setInterval(() => {
      setTel(prev => ({
        battery:   Math.max(0, Math.min(100, prev.battery - 0.05)),
        signal:    Math.max(0, Math.min(100, prev.signal + (Math.random() - 0.5) * 3)),
        motorTemp: Math.max(30, Math.min(85, prev.motorTemp + (Math.random() - 0.45) * 0.8)),
        speed:     prev.speed,
        gear:      prev.gear,
      }));
    }, 2000);
    return () => clearInterval(id);
  }, []);
  return [tel, setTel];
}

const sensorColor = (v) =>
  v > 1.5 ? "#22c55e" : v >= 0.5 ? "#f59e0b" : "#ef4444";
const sensorLabel = (v) =>
  v > 1.5 ? "CLEAR" : v >= 0.5 ? "CAUTION" : "DANGER";

// Shared components
function Card({ children, className = "", glow = false, style = {} }) {
  return (
    <div
      className={`rounded-2xl border border-white/10 bg-white/5 backdrop-blur-sm p-4 ${className}`}
      style={{
        boxShadow: glow ? "0 0 20px rgba(56,189,248,0.15), inset 0 1px 0 rgba(255,255,255,0.08)" : "inset 0 1px 0 rgba(255,255,255,0.06)",
        ...style
      }}
    >
      {children}
    </div>
  );
}

function SensorCard({ label, value, icon: Icon }) {
  const color = sensorColor(value);
  const status = sensorLabel(value);
  const ring = value < 0.5 ? "animate-pulse" : "";
  return (
    <Card className={`flex flex-col gap-2 ${ring}`} style={{ borderColor: `${color}33` }}>
      <div className="flex items-center justify-between">
        <span className="text-xs font-mono text-slate-400 tracking-widest">{label}</span>
        <Icon size={14} style={{ color }} />
      </div>
      <div className="text-3xl font-mono font-bold" style={{ color }}>
        {value.toFixed(1)}<span className="text-lg text-slate-500">m</span>
      </div>
      <div className="flex items-center gap-2">
        <div className="h-1 flex-1 rounded-full bg-white/10">
          <div
            className="h-1 rounded-full transition-all duration-700"
            style={{ width: `${Math.min(100, (value / 4) * 100)}%`, background: color }}
          />
        </div>
        <span className="text-[10px] font-mono font-bold" style={{ color }}>{status}</span>
      </div>
    </Card>
  );
}

function EmergencyStop({ onStop, active }) {
  return (
    <motion.button
      whileTap={{ scale: 0.92 }}
      onClick={onStop}
      className={`flex items-center gap-2 px-4 py-2 rounded-xl font-bold text-sm font-mono tracking-widest transition-all
        ${active
          ? "bg-red-600 text-white border-2 border-red-400 shadow-lg shadow-red-900/60 animate-pulse"
          : "bg-red-900/60 text-red-400 border border-red-700/60 hover:bg-red-700/60"
        }`}
    >
      <Square size={14} fill="currentColor" />
      E-STOP
    </motion.button>
  );
}

function Sidebar({ page, setPage }) {
  const nav = [
    { id: "dashboard",  icon: LayoutDashboard, label: "Dashboard" },
    { id: "camera",     icon: Camera,          label: "Camera" },
    { id: "control",    icon: Crosshair,       label: "Control" },
    { id: "sensors",    icon: Radio,           label: "Sensors" },
    { id: "autopark",   icon: ParkingSquare,   label: "Auto Park" },
    { id: "alerts",     icon: Bell,            label: "Alerts" },
    { id: "settings",   icon: Settings,        label: "Settings" },
  ];
  return (
    <aside className="hidden md:flex flex-col w-16 lg:w-56 bg-[#060d1a] border-r border-white/8 shrink-0">
      <div className="h-14 flex items-center justify-center lg:justify-start lg:px-5 border-b border-white/8">
        <Car size={20} className="text-sky-400 shrink-0" />
        <span className="hidden lg:block ml-2 font-mono font-bold text-sky-400 tracking-widest text-sm">E-CAR</span>
      </div>
      <nav className="flex flex-col gap-1 p-2 flex-1">
        {nav.map(({ id, icon: Icon, label }) => (
          <button
            key={id}
            onClick={() => setPage(id)}
            className={`flex items-center gap-3 px-3 py-2.5 rounded-xl transition-all text-sm font-mono
              ${page === id
                ? "bg-sky-500/20 text-sky-400 border border-sky-500/40"
                : "text-slate-500 hover:text-slate-200 hover:bg-white/5"
              }`}
          >
            <Icon size={16} className="shrink-0" />
            <span className="hidden lg:block">{label}</span>
          </button>
        ))}
      </nav>
      <div className="p-3 border-t border-white/8">
        <div className="hidden lg:flex items-center gap-2 px-2">
          <div className="w-2 h-2 rounded-full bg-green-400 animate-pulse" />
          <span className="text-[10px] font-mono text-slate-500">v2.4.1 - ONLINE</span>
        </div>
      </div>
    </aside>
  );
}

function TopBar({ tel, sensors, onStop, emergencyActive, setPage, page }) {
  const battColor = tel.battery > 50 ? "#22c55e" : tel.battery > 20 ? "#f59e0b" : "#ef4444";
  const critSensor = Object.values(sensors).some(v => v < 0.5);
  const nav = [
    { id: "dashboard",  icon: LayoutDashboard },
    { id: "camera",     icon: Camera },
    { id: "control",    icon: Crosshair },
    { id: "sensors",    icon: Radio },
    { id: "autopark",   icon: ParkingSquare },
    { id: "alerts",     icon: Bell },
    { id: "settings",   icon: Settings },
  ];
  return (
    <header className="h-14 flex items-center justify-between px-4 bg-[#060d1a]/90 backdrop-blur border-b border-white/8 shrink-0">
      <div className="flex items-center gap-3">
        <Car size={18} className="text-sky-400 md:hidden" />
        <span className="font-mono text-sky-400 font-bold tracking-widest text-xs md:text-sm">E-CAR REMOTE</span>
      </div>
      {/* Mobile nav */}
      <div className="flex md:hidden gap-1 overflow-x-auto">
        {nav.map(({ id, icon: Icon }) => (
          <button key={id} onClick={() => setPage(id)}
            className={`p-2 rounded-lg text-xs transition-all ${page === id ? "text-sky-400 bg-sky-500/20" : "text-slate-500"}`}>
            <Icon size={15} />
          </button>
        ))}
      </div>
      <div className="hidden md:flex items-center gap-4">
        <div className="flex items-center gap-1.5">
          <div className="w-2 h-2 rounded-full bg-green-400 animate-pulse" />
          <span className="text-xs font-mono text-green-400">CONNECTED</span>
        </div>
        <div className="flex items-center gap-1">
          <Wifi size={14} style={{ color: tel.signal > 50 ? "#22c55e" : "#f59e0b" }} />
          <span className="text-xs font-mono text-slate-400">{Math.round(tel.signal)}%</span>
        </div>
        <div className="flex items-center gap-1">
          <Battery size={14} style={{ color: battColor }} />
          <span className="text-xs font-mono" style={{ color: battColor }}>{Math.round(tel.battery)}%</span>
        </div>
        {critSensor && (
          <div className="flex items-center gap-1 bg-red-900/40 px-2 py-0.5 rounded-lg border border-red-700/60 animate-pulse">
            <AlertTriangle size={12} className="text-red-400" />
            <span className="text-[10px] font-mono text-red-400">OBSTACLE</span>
          </div>
        )}
        <EmergencyStop onStop={onStop} active={emergencyActive} />
      </div>
    </header>
  );
}

// Page: login
function LoginPage({ onLogin }) {
  const [pin, setPin] = useState("");
  const [shake, setShake] = useState(false);
  const handlePin = (d) => {
    if (pin.length >= 4) return;
    const next = pin + d;
    setPin(next);
    if (next.length === 4) {
      if (next === "1234") setTimeout(onLogin, 300);
      else {
        setTimeout(() => { setShake(true); setPin(""); setTimeout(() => setShake(false), 600); }, 200);
      }
    }
  };
  return (
    <div className="min-h-screen bg-[#030812] flex flex-col items-center justify-center relative overflow-hidden">
      {/* Grid bg */}
      <div className="absolute inset-0 opacity-10"
        style={{ backgroundImage: "linear-gradient(rgba(56,189,248,0.3) 1px,transparent 1px),linear-gradient(90deg,rgba(56,189,248,0.3) 1px,transparent 1px)", backgroundSize: "40px 40px" }} />
      <div className="absolute top-1/3 left-1/2 -translate-x-1/2 -translate-y-1/2 w-96 h-96 bg-sky-600/10 rounded-full blur-3xl" />
      <motion.div initial={{ opacity: 0, y: 20 }} animate={{ opacity: 1, y: 0 }} className="relative z-10 w-80">
        <div className="text-center mb-8">
          <div className="inline-flex items-center justify-center w-16 h-16 rounded-2xl bg-sky-500/20 border border-sky-500/40 mb-4">
            <Car size={28} className="text-sky-400" />
          </div>
          <h1 className="text-2xl font-mono font-bold text-white tracking-widest">E-CAR REMOTE</h1>
          <p className="text-slate-500 text-sm mt-1 font-mono">SECURE ACCESS PORTAL</p>
        </div>
        <Card glow>
          <p className="text-center text-xs font-mono text-slate-400 mb-4 tracking-widest">ENTER PIN CODE</p>
          <motion.div
            animate={shake ? { x: [-6, 6, -6, 6, 0] } : {}}
            className="flex justify-center gap-3 mb-6"
          >
            {[0,1,2,3].map(i => (
              <div key={i} className={`w-3 h-3 rounded-full border-2 transition-all duration-200
                ${pin.length > i ? "bg-sky-400 border-sky-400" : "border-slate-600 bg-transparent"}`} />
            ))}
          </motion.div>
          <div className="grid grid-cols-3 gap-2">
            {[1,2,3,4,5,6,7,8,9,"",0,"DEL"].map((d, i) => (
              <button key={i}
                onClick={() => d === "DEL" ? setPin(p => p.slice(0,-1)) : d !== "" && handlePin(String(d))}
                className={`h-12 rounded-xl font-mono font-bold text-lg transition-all
                  ${d === "" ? "" :
                    d === "DEL" ? "text-slate-400 hover:text-white bg-white/5 hover:bg-white/10 active:scale-95"
                    : "text-white bg-white/8 hover:bg-sky-500/20 hover:text-sky-300 active:scale-95 border border-white/8 hover:border-sky-500/30"
                  }`}
              >
                {d}
              </button>
            ))}
          </div>
          <p className="text-center text-[10px] font-mono text-slate-600 mt-4">DEMO PIN: 1234</p>
        </Card>
      </motion.div>
    </div>
  );
}

// Page: dashboard
function DashboardPage({ sensors, tel, setPage }) {
  return (
    <div className="grid grid-cols-1 md:grid-cols-2 xl:grid-cols-3 gap-4 p-4 overflow-auto">
      {/* Camera preview */}
      <Card glow className="md:col-span-2 xl:col-span-2 min-h-48 relative overflow-hidden cursor-pointer" onClick={() => setPage("camera")}>
        <div className="absolute inset-0 bg-gradient-to-b from-slate-900/90 to-slate-950/90 rounded-2xl" />
        <div className="absolute inset-0 flex items-center justify-center opacity-20"
          style={{ backgroundImage: "repeating-linear-gradient(0deg,rgba(56,189,248,0.2) 0px,transparent 2px,transparent 24px)", backgroundSize: "24px 24px" }} />
        {/* Guide lines */}
        <svg className="absolute inset-0 w-full h-full" viewBox="0 0 400 200" preserveAspectRatio="none">
          <line x1="200" y1="200" x2="80" y2="80" stroke="#22c55e" strokeWidth="1.5" strokeDasharray="6 4" opacity="0.8" />
          <line x1="200" y1="200" x2="320" y2="80" stroke="#22c55e" strokeWidth="1.5" strokeDasharray="6 4" opacity="0.8" />
          <ellipse cx="200" cy="180" rx="60" ry="12" stroke="#22c55e" strokeWidth="1" fill="none" opacity="0.4" />
          <line x1="0" y1="140" x2="400" y2="140" stroke="#f59e0b" strokeWidth="0.5" opacity="0.3" />
        </svg>
        <div className="relative z-10 flex flex-col h-full min-h-40 justify-between">
          <div className="flex justify-between items-start">
            <span className="text-[10px] font-mono text-slate-400 bg-black/40 px-2 py-0.5 rounded">REAR CAM</span>
            <div className="flex items-center gap-1 bg-red-600/80 px-2 py-0.5 rounded text-[10px] font-mono font-bold text-white">
              <div className="w-1.5 h-1.5 rounded-full bg-white animate-pulse" /> REC
            </div>
          </div>
          <div className="text-center mt-4">
            <Camera size={32} className="text-slate-600 mx-auto mb-1" />
            <p className="text-xs font-mono text-slate-500">Click to open Camera Assist</p>
          </div>
          <div className="flex justify-between text-[10px] font-mono text-slate-500">
            <span>FOV 120°</span><span>12ms</span><span>1080p</span>
          </div>
        </div>
      </Card>

      {/* Telemetry */}
      <Card className="flex flex-col gap-3">
        <p className="text-[10px] font-mono text-slate-400 tracking-widest">TELEMETRY</p>
        {[
          { label: "BATTERY", value: `${Math.round(tel.battery)}%`, color: tel.battery > 50 ? "#22c55e" : "#f59e0b", icon: Battery },
          { label: "SIGNAL",  value: `${Math.round(tel.signal)}%`,  color: "#38bdf8", icon: Wifi },
          { label: "MOTOR TEMP", value: `${tel.motorTemp.toFixed(1)}°C`, color: tel.motorTemp > 70 ? "#ef4444" : "#f59e0b", icon: Thermometer },
          { label: "SPEED",   value: `${tel.speed.toFixed(1)} m/s`, color: "#a78bfa", icon: Gauge },
          { label: "GPS",     value: `${MOCK_GPS.lat.toFixed(4)}N`, color: "#34d399", icon: MapPin },
        ].map(({ label, value, color, icon: Icon }) => (
          <div key={label} className="flex items-center justify-between">
            <div className="flex items-center gap-2">
              <Icon size={13} style={{ color }} />
              <span className="text-[10px] font-mono text-slate-500">{label}</span>
            </div>
            <span className="text-xs font-mono font-bold" style={{ color }}>{value}</span>
          </div>
        ))}
      </Card>

      {/* Sensor summary */}
      {["front","rear","left","right"].map(k => (
        <SensorCard
          key={k}
          label={k.toUpperCase()}
          value={sensors[k]}
          icon={Activity}
        />
      ))}

      {/* Alerts feed */}
      <Card className="md:col-span-2 xl:col-span-3">
        <p className="text-[10px] font-mono text-slate-400 tracking-widest mb-3">LIVE ALERTS</p>
        <div className="flex flex-col gap-2">
          {sensors.right < 0.5 && (
            <div className="flex items-center gap-3 bg-red-950/40 border border-red-800/50 rounded-xl px-3 py-2 animate-pulse">
              <AlertTriangle size={14} className="text-red-400 shrink-0" />
              <span className="text-xs font-mono text-red-300">DANGER - Right obstacle: {sensors.right.toFixed(2)}m</span>
              <span className="ml-auto text-[10px] font-mono text-red-600">NOW</span>
            </div>
          )}
          {sensors.front < 1.5 && (
            <div className="flex items-center gap-3 bg-amber-950/40 border border-amber-800/40 rounded-xl px-3 py-2">
              <AlertTriangle size={14} className="text-amber-400 shrink-0" />
              <span className="text-xs font-mono text-amber-300">WARNING - Front obstacle: {sensors.front.toFixed(2)}m</span>
              <span className="ml-auto text-[10px] font-mono text-amber-700">~2s</span>
            </div>
          )}
          <div className="flex items-center gap-3 bg-sky-950/30 border border-sky-800/30 rounded-xl px-3 py-2">
            <Zap size={14} className="text-sky-400 shrink-0" />
            <span className="text-xs font-mono text-sky-300">INFO - Auto-park ready. Spot B2-14 available.</span>
            <span className="ml-auto text-[10px] font-mono text-sky-700">5m ago</span>
          </div>
          <div className="flex items-center gap-3 bg-green-950/30 border border-green-800/30 rounded-xl px-3 py-2">
            <Battery size={14} className="text-green-400 shrink-0" />
            <span className="text-xs font-mono text-green-300">INFO - Battery level normal ({Math.round(tel.battery)}%)</span>
            <span className="ml-auto text-[10px] font-mono text-green-700">1m ago</span>
          </div>
        </div>
      </Card>
    </div>
  );
}

// Page: camera
function CameraPage({ sensors }) {
  const [latency] = useState(12);
  return (
    <div className="flex flex-col gap-4 p-4 overflow-auto">
      <Card glow className="relative overflow-hidden" style={{ minHeight: 360 }}>
        {/* Garage background simulation */}
        <div className="absolute inset-0 rounded-2xl overflow-hidden">
          <div className="w-full h-full bg-gradient-to-b from-[#0a1020] via-[#0d1830] to-[#060c18]" />
          {/* Ceiling lights */}
          {[20,50,80].map(x => (
            <div key={x} className="absolute top-0 h-16 w-1 bg-gradient-to-b from-yellow-300/20 to-transparent rounded-b-full"
              style={{ left: `${x}%`, filter: "blur(4px)" }} />
          ))}
          {/* Floor markings */}
          <svg className="absolute inset-0 w-full h-full" viewBox="0 0 800 400" preserveAspectRatio="none">
            {/* Perspective grid */}
            {[-3,-2,-1,0,1,2,3].map(i => (
              <line key={i} x1={400+i*120} y1={0} x2={400+i*400} y2={400}
                stroke="rgba(56,189,248,0.08)" strokeWidth="1" />
            ))}
            {[0,1,2,3,4].map(i => (
              <line key={i} x1={0} y1={i*100} x2={800} y2={i*100}
                stroke="rgba(56,189,248,0.05)" strokeWidth="0.5" />
            ))}
            {/* Parking guide lines */}
            <line x1="400" y1="400" x2="160" y2="120" stroke="#22c55e" strokeWidth="2" strokeDasharray="12 6" opacity="0.9" />
            <line x1="400" y1="400" x2="640" y2="120" stroke="#22c55e" strokeWidth="2" strokeDasharray="12 6" opacity="0.9" />
            {/* Distance arcs */}
            <ellipse cx="400" cy="380" rx="80"  ry="15" stroke="#22c55e" strokeWidth="1.5" fill="none" opacity="0.7" />
            <ellipse cx="400" cy="380" rx="160" ry="30" stroke="#f59e0b" strokeWidth="1"   fill="none" opacity="0.5" />
            <ellipse cx="400" cy="380" rx="240" ry="45" stroke="#ef4444" strokeWidth="0.5" fill="none" opacity="0.3" />
            {/* Obstacle box right */}
            <rect x="600" y="200" width="80" height="120" fill="rgba(239,68,68,0.15)" stroke="#ef4444" strokeWidth="1.5" rx="4" />
            <text x="640" y="195" textAnchor="middle" fill="#ef4444" fontSize="11" fontFamily="monospace">{sensors.right.toFixed(1)}m</text>
            {/* Obstacle box left (amber) */}
            {sensors.left < 1.5 && (
              <>
                <rect x="120" y="220" width="70" height="100" fill="rgba(245,158,11,0.15)" stroke="#f59e0b" strokeWidth="1.5" rx="4" />
                <text x="155" y="215" textAnchor="middle" fill="#f59e0b" fontSize="11" fontFamily="monospace">{sensors.left.toFixed(1)}m</text>
              </>
            )}
            {/* Center cross */}
            <line x1="395" y1="360" x2="405" y2="360" stroke="#38bdf8" strokeWidth="1.5" />
            <line x1="400" y1="355" x2="400" y2="365" stroke="#38bdf8" strokeWidth="1.5" />
          </svg>
        </div>
        {/* HUD overlay */}
        <div className="relative z-10 flex flex-col h-full min-h-80 justify-between pointer-events-none">
          <div className="flex justify-between items-start">
            <div className="flex gap-2">
              <span className="text-[10px] font-mono bg-black/60 text-slate-300 px-2 py-1 rounded border border-white/10">REAR CAM</span>
              <span className="text-[10px] font-mono bg-black/60 text-sky-400 px-2 py-1 rounded border border-sky-500/30">FOV 120°</span>
            </div>
            <div className="flex items-center gap-1 bg-red-600/90 px-2 py-1 rounded text-[10px] font-mono font-bold text-white">
              <div className="w-1.5 h-1.5 rounded-full bg-white animate-pulse" /> REC
            </div>
          </div>
          <div className="flex justify-between items-end">
            <span className="text-[10px] font-mono bg-black/60 text-green-400 px-2 py-1 rounded">{latency}ms LATENCY</span>
            <span className="text-[10px] font-mono bg-black/60 text-slate-400 px-2 py-1 rounded">1080p · 30fps</span>
          </div>
        </div>
      </Card>

      {/* Sensor overlays */}
      <div className="grid grid-cols-2 md:grid-cols-4 gap-3">
        {Object.entries(sensors).map(([k, v]) => (
          <SensorCard key={k} label={k.toUpperCase()} value={v} icon={Radio} />
        ))}
      </div>

      <div className="grid grid-cols-3 gap-3">
        {[
          { label: "FRONT CLEAR", color: sensorColor(sensors.front), val: sensors.front.toFixed(1) + "m" },
          { label: "REAR CLEAR",  color: sensorColor(sensors.rear),  val: sensors.rear.toFixed(1)  + "m" },
          { label: "OBSTACLE R",  color: sensorColor(sensors.right), val: sensors.right.toFixed(1) + "m" },
        ].map(({ label, color, val }) => (
          <div key={label} className="text-center py-3 rounded-xl border" style={{ borderColor: `${color}40`, background: `${color}10` }}>
            <div className="text-xl font-mono font-bold" style={{ color }}>{val}</div>
            <div className="text-[10px] font-mono text-slate-500 mt-0.5">{label}</div>
          </div>
        ))}
      </div>
    </div>
  );
}

// Page: manual control
function ControlPage({ tel, setTel }) {
  const [active, setActive] = useState(null);
  const [speed, setSpeed] = useState(1.2);
  const [mode, setMode] = useState("Drive");
  const dirs = { up: "FORWARD", down: "REVERSE", left: "LEFT", right: "RIGHT" };

  const handleDir = (dir, down) => {
    setActive(down ? dir : null);
    if (down) {
      setTel(t => ({
        ...t,
        speed: dir === "down" ? -speed : dir === "up" ? speed : speed * 0.35,
        gear: dir === "down" ? "R" : "D"
      }));
    } else {
      setTel(t => ({ ...t, speed: 0 }));
    }
  };

  const handleStop = () => {
    setActive(null);
    setTel(t => ({ ...t, speed: 0, gear: "P" }));
  };

  const DBtn = ({ dir, children }) => (
    <motion.button
      whileTap={{ scale: 0.9 }}
      onPointerDown={() => handleDir(dir, true)}
      onPointerUp={() => handleDir(dir, false)}
      onPointerLeave={() => handleDir(dir, false)}
      className={`manual-dpad-btn ${active === dir ? "active" : ""}`}
    >
      {children}
    </motion.button>
  );

  const ModeButton = ({ label, value }) => (
    <button
      onClick={() => setMode(value)}
      className={`manual-mode ${value.toLowerCase()} ${mode === value ? "active" : ""}`}
    >
      {label}
    </button>
  );

  return (
    <div className="p-4 overflow-auto">
      <Card glow className="manual-control-page">
        <p className="manual-page-title">4. MANUAL CONTROL PAGE</p>
        <div className="manual-control-layout">
          <div className="manual-steering">
            <div className="manual-wheel">
              <img src={steeringWheel} alt="Electric vehicle steering wheel" />
            </div>
          </div>

          <div className="manual-speed">
            <p className="manual-label">MAX SPEED</p>
            <strong>{speed.toFixed(1)} m/s</strong>
            <div className="manual-slider-wrap">
              <input
                type="range"
                min="0"
                max="2"
                step="0.1"
                value={speed}
                onChange={e => setSpeed(Number(e.target.value))}
                className="manual-vertical-slider"
                aria-label="Max speed"
              />
              <div className="manual-scale">
                <span>2.0</span>
                <span>1.5</span>
                <span>1.0</span>
                <span>0.5</span>
                <span>0.0</span>
              </div>
            </div>
          </div>

          <div className="manual-controls">
            <div className="manual-dpad">
              <div />
              <DBtn dir="up"><ChevronUp size={25} fill="currentColor" /></DBtn>
              <div />
              <DBtn dir="left"><ChevronLeft size={25} fill="currentColor" /></DBtn>
              <div className="manual-dpad-core" />
              <DBtn dir="right"><ChevronRight size={25} fill="currentColor" /></DBtn>
              <div />
              <DBtn dir="down"><ChevronDown size={25} fill="currentColor" /></DBtn>
              <div />
            </div>
            <motion.button whileTap={{ scale: 0.95 }} onClick={handleStop} className="manual-stop">
              STOP
            </motion.button>
          </div>
        </div>

        <div className="manual-bottom">
          <ModeButton label="DRIVE" value="Drive" />
          <ModeButton label="CREEP" value="Creep" />
          <ModeButton label="PRECISION" value="Precision" />
        </div>
        {active && <div className="manual-active">{dirs[active]} ACTIVE</div>}
      </Card>
    </div>
  );
}

// Page: sensors
function SensorsPage({ sensors }) {
  const [tick, setTick] = useState(0);
  useEffect(() => { const id = setInterval(() => setTick(t => t + 1), 800); return () => clearInterval(id); }, []);
  const zone = (value) => value > 1.5 ? "safe" : value >= 0.5 ? "caution" : "danger";
  const zoneText = (value) => value > 1.5 ? "SAFE" : value >= 0.5 ? "CAUTION" : "DANGER";
  const sensorTiles = [
    { key: "front", label: "FRONT" },
    { key: "rear", label: "REAR" },
    { key: "left", label: "LEFT" },
    { key: "right", label: "RIGHT" },
  ];

  return (
    <div className="flex flex-col gap-4 p-4 overflow-auto">
      <Card glow className="sensor-analytics-page">
        <p className="sensor-analytics-title">5. SENSOR ANALYTICS PAGE</p>
        <div className="sensor-analytics-stage">
          <div className="sensor-tile-grid left">
            {sensorTiles.filter(item => item.key === "front" || item.key === "left").map(({ key, label }) => (
              <div key={key} className={`sensor-analytics-tile ${zone(sensors[key])}`}>
                <span>{label}</span>
                <strong>{sensors[key].toFixed(1)}m</strong>
                <Radio size={18} />
              </div>
            ))}
          </div>

          <div className="sensor-car-zone">
            <div className="sonar-wave-stack front">
              <span /><span /><span />
            </div>
            <div className="sonar-wave-stack left">
              <span /><span /><span />
            </div>
            <div className="sonar-wave-stack right">
              <span /><span /><span />
            </div>
            <div className="sonar-bottom-arc" />
            <div className="realistic-car-top analytics">
              <div className="car-glass front" />
              <div className="car-glass rear" />
              <div className="car-light l1" />
              <div className="car-light l2" />
            </div>
          </div>

          <div className="sensor-tile-grid right">
            {sensorTiles.filter(item => item.key === "rear" || item.key === "right").map(({ key, label }) => (
              <div key={key} className={`sensor-analytics-tile ${zone(sensors[key])}`}>
                <span>{label}</span>
                <strong>{sensors[key].toFixed(1)}m</strong>
                <Radio size={18} />
              </div>
            ))}
          </div>
        </div>
      </Card>

      <div className="grid grid-cols-1 md:grid-cols-4 gap-3">
        {sensorTiles.map(({ key, label }) => (
          <Card key={key} className={`sensor-status-card ${zone(sensors[key])}`}>
            <div className="flex items-center justify-between">
              <span className="text-[10px] font-mono text-slate-400 tracking-widest">{label} SENSOR</span>
              <Radio size={15} />
            </div>
            <div className="text-3xl font-mono font-bold mt-3">{sensors[key].toFixed(1)}<span className="text-base text-slate-500">m</span></div>
            <p className="text-[10px] font-mono font-bold mt-2">{zoneText(sensors[key])}</p>
            <div className="h-1.5 bg-white/10 rounded-full mt-3 overflow-hidden">
              <div className="h-full rounded-full" style={{ width: `${Math.min(100, (sensors[key] / 3) * 100)}%` }} />
            </div>
          </Card>
        ))}
      </div>
    </div>
  );
}

// Page: auto park
function AutoParkPage() {
  const [status, setStatus] = useState("READY");
  const [progress, setProgress] = useState(0);
  const [eta, setEta] = useState(45);
  const intervalRef = useRef(null);

  const start = () => {
    setStatus("PARKING");
    intervalRef.current = setInterval(() => {
      setProgress(p => {
        if (p >= 100) { clearInterval(intervalRef.current); setStatus("PARKED"); setEta(0); return 100; }
        setEta(e => Math.max(0, e - 1));
        return p + 2.5;
      });
    }, 300);
  };
  const pause = () => { clearInterval(intervalRef.current); setStatus("PAUSED"); };
  const cancel = () => { clearInterval(intervalRef.current); setStatus("READY"); setProgress(0); setEta(45); };

  const spots = Array.from({ length: 24 }, (_, i) => {
    const id = `B2-${(i + 1).toString().padStart(2, "0")}`;
    const isCurrent = id === "B2-07";
    const isTarget = id === "B2-14";
    const occupied = [1,2,3,5,8,9,11,12,15,16,17,19,20,21,22,23].includes(i + 1);
    return { id, isCurrent, isTarget, occupied };
  });

  return (
    <div className="flex flex-col md:flex-row gap-4 p-4 overflow-auto">
      {/* Parking map */}
      <Card glow className="flex-1 min-h-64">
        <p className="text-[10px] font-mono text-slate-400 tracking-widest mb-4">PARKING MAP - LEVEL B2</p>
        <div className="grid grid-cols-6 gap-1.5 mb-4">
          {spots.map(({ id, isCurrent, isTarget, occupied }) => (
            <div key={id}
              className={`h-10 rounded-lg flex flex-col items-center justify-center text-[8px] font-mono border transition-all relative
                ${isCurrent  ? "bg-green-500/20 border-green-500/60 text-green-400" :
                  isTarget   ? "bg-sky-500/30 border-sky-400 text-sky-300 animate-pulse" :
                  occupied   ? "bg-red-900/30 border-red-800/40 text-red-600" :
                               "bg-white/4 border-white/8 text-slate-600"
                }`}
            >
              <span className="font-bold text-[9px]">{id.split("-")[1]}</span>
              {isCurrent && <Car size={10} className="text-green-400" />}
              {isTarget  && <Crosshair size={10} className="text-sky-400" />}
            </div>
          ))}
        </div>
        <div className="flex gap-3 text-[10px] font-mono flex-wrap">
          {[
            { color: "#22c55e", label: "YOUR CAR" },
            { color: "#38bdf8", label: "TARGET B2-14" },
            { color: "#ef4444", label: "OCCUPIED" },
            { color: "#ffffff22", label: "FREE" },
          ].map(({ color, label }) => (
            <div key={label} className="flex items-center gap-1.5">
              <div className="w-3 h-3 rounded" style={{ background: color }} />
              <span className="text-slate-500">{label}</span>
            </div>
          ))}
        </div>
      </Card>

      {/* Info + controls */}
      <div className="flex flex-col gap-4 w-full md:w-64">
        <Card className="flex flex-col gap-3">
          <p className="text-[10px] font-mono text-slate-400 tracking-widest">INFO</p>
          {[
            { label: "TARGET",   value: "B2-14",    color: "#38bdf8" },
            { label: "DISTANCE", value: "18.5m",    color: "#a78bfa" },
            { label: "ETA",      value: `${eta}s`,  color: "#f59e0b" },
            { label: "STATUS",   value: status,     color: status === "PARKED" ? "#22c55e" : status === "PARKING" ? "#38bdf8" : status === "PAUSED" ? "#f59e0b" : "#94a3b8" },
          ].map(({ label, value, color }) => (
            <div key={label} className="flex justify-between items-center">
              <span className="text-[10px] font-mono text-slate-500">{label}</span>
              <span className="text-sm font-mono font-bold" style={{ color }}>{value}</span>
            </div>
          ))}
          {status === "PARKING" && (
            <div className="mt-1">
              <div className="flex justify-between text-[10px] font-mono text-slate-500 mb-1">
                <span>PROGRESS</span><span>{Math.round(progress)}%</span>
              </div>
              <div className="h-1.5 rounded-full bg-white/10">
                <div className="h-1.5 rounded-full bg-sky-500 transition-all duration-300" style={{ width: `${progress}%` }} />
              </div>
            </div>
          )}
        </Card>

        <div className="flex flex-col gap-2">
          <motion.button whileTap={{ scale: 0.95 }}
            onClick={status === "READY" || status === "PAUSED" ? start : undefined}
            disabled={status === "PARKING" || status === "PARKED"}
            className={`flex items-center justify-center gap-2 py-3 rounded-xl font-mono font-bold text-xs border transition-all
              ${status === "READY" || status === "PAUSED"
                ? "bg-green-500/20 border-green-500/50 text-green-400 hover:bg-green-500/30"
                : "bg-white/4 border-white/8 text-slate-600 cursor-not-allowed"
              }`}
          >
            <PlayCircle size={15} /> START AUTO PARK
          </motion.button>
          <motion.button whileTap={{ scale: 0.95 }}
            onClick={status === "PARKING" ? pause : undefined}
            className={`flex items-center justify-center gap-2 py-3 rounded-xl font-mono font-bold text-xs border transition-all
              ${status === "PARKING"
                ? "bg-amber-500/20 border-amber-500/50 text-amber-400 hover:bg-amber-500/30"
                : "bg-white/4 border-white/8 text-slate-600 cursor-not-allowed"
              }`}
          >
            <PauseCircle size={15} /> PAUSE
          </motion.button>
          <motion.button whileTap={{ scale: 0.95 }}
            onClick={status !== "READY" ? cancel : undefined}
            className={`flex items-center justify-center gap-2 py-3 rounded-xl font-mono font-bold text-xs border transition-all
              ${status !== "READY"
                ? "bg-red-500/20 border-red-500/50 text-red-400 hover:bg-red-500/30"
                : "bg-white/4 border-white/8 text-slate-600 cursor-not-allowed"
              }`}
          >
            <XCircle size={15} /> CANCEL
          </motion.button>
        </div>
      </div>
    </div>
  );
}

// Page: alerts
function AlertsPage({ sensors, tel }) {
  const alerts = [
    sensors.right < 0.5   && { level: "DANGER",  color: "#ef4444", bg: "bg-red-950/40",    border: "border-red-800/50",    icon: AlertTriangle, msg: `Obstacle on right - ${sensors.right.toFixed(2)}m`,   time: "NOW",   icon2: AlertTriangle },
    sensors.front < 1.5   && { level: "WARNING", color: "#f59e0b", bg: "bg-amber-950/40",  border: "border-amber-800/40",  icon: AlertTriangle, msg: `Front obstacle detected - ${sensors.front.toFixed(2)}m`, time: "~2s", icon2: AlertTriangle },
    sensors.left < 1.5    && { level: "WARNING", color: "#f59e0b", bg: "bg-amber-950/40",  border: "border-amber-800/40",  icon: AlertTriangle, msg: `Left obstacle detected - ${sensors.left.toFixed(2)}m`,  time: "~5s", icon2: AlertTriangle },
    { level: "INFO",  color: "#38bdf8", bg: "bg-sky-950/30",    border: "border-sky-800/30",    icon: Zap,         msg: "Auto-park ready. Spot B2-14 available.",              time: "5m ago" },
    { level: "INFO",  color: "#22c55e", bg: "bg-green-950/30",  border: "border-green-800/30",  icon: Battery,     msg: `Battery level normal (${Math.round(tel.battery)}%)`,  time: "1m ago" },
    { level: "INFO",  color: "#a78bfa", bg: "bg-violet-950/30", border: "border-violet-800/30", icon: Wifi,        msg: `Signal strength ${Math.round(tel.signal)}% - stable`, time: "2m ago" },
  ].filter(Boolean);

  return (
    <div className="flex flex-col gap-3 p-4 overflow-auto">
      <div className="flex items-center justify-between mb-1">
        <p className="text-[10px] font-mono text-slate-400 tracking-widest">ALERT FEED</p>
        <span className="text-[10px] font-mono text-slate-600">{alerts.length} active</span>
      </div>
      <AnimatePresence>
        {alerts.map((a, i) => {
          const Icon = a.icon;
          return (
            <motion.div key={a.msg} initial={{ opacity: 0, x: -16 }} animate={{ opacity: 1, x: 0 }} transition={{ delay: i * 0.06 }}>
              <div className={`flex items-start gap-3 ${a.bg} ${a.border} border rounded-xl px-4 py-3 ${a.level === "DANGER" ? "animate-pulse" : ""}`}>
                <Icon size={15} className="shrink-0 mt-0.5" style={{ color: a.color }} />
                <div className="flex-1 min-w-0">
                  <div className="flex items-center gap-2 mb-0.5">
                    <span className="text-[10px] font-mono font-bold px-1.5 py-0.5 rounded" style={{ color: a.color, background: `${a.color}18` }}>{a.level}</span>
                  </div>
                  <p className="text-xs font-mono" style={{ color: a.color }}>{a.msg}</p>
                </div>
                <span className="text-[10px] font-mono shrink-0" style={{ color: `${a.color}80` }}>{a.time}</span>
              </div>
            </motion.div>
          );
        })}
      </AnimatePresence>
    </div>
  );
}

// Page: settings
function SettingsPage() {
  const [vals, setVals] = useState({
    maxSpeed: 1.5, stopDist: 0.4, sensorSens: 75, nightMode: true,
    camOverlay: true, twoFactor: false,
  });
  const toggle = k => setVals(v => ({ ...v, [k]: !v[k] }));
  const slider = (k, val) => setVals(v => ({ ...v, [k]: val }));

  const Toggle = ({ k }) => (
    <button onClick={() => toggle(k)} className="flex items-center gap-1">
      {vals[k]
        ? <ToggleRight size={22} className="text-sky-400" />
        : <ToggleLeft size={22} className="text-slate-600" />
      }
    </button>
  );

  const sections = [
    {
      title: "VEHICLE",
      items: [
        { label: "Max speed limit", sub: `${vals.maxSpeed.toFixed(1)} m/s`, type: "range", k: "maxSpeed", min: 0.2, max: 2, step: 0.1 },
        { label: "Auto-stop distance", sub: `${vals.stopDist.toFixed(2)} m`, type: "range", k: "stopDist", min: 0.1, max: 1.0, step: 0.05 },
      ]
    },
    {
      title: "SENSORS",
      items: [
        { label: "Sensor sensitivity", sub: `${vals.sensorSens}%`, type: "range", k: "sensorSens", min: 0, max: 100, step: 5 },
      ]
    },
    {
      title: "CAMERA",
      items: [
        { label: "Camera overlay",      sub: vals.camOverlay ? "On" : "Off", type: "toggle", k: "camOverlay" },
      ]
    },
    {
      title: "CONTROL",
      items: [
        { label: "Night mode",          sub: vals.nightMode ? "On" : "Off", type: "toggle", k: "nightMode" },
      ]
    },
    {
      title: "SECURITY",
      items: [
        { label: "Two-factor confirm",  sub: vals.twoFactor ? "On" : "Off", type: "toggle", k: "twoFactor" },
      ]
    },
  ];

  return (
    <div className="flex flex-col gap-4 p-4 overflow-auto">
      {sections.map(({ title, items }) => (
        <Card key={title} className="flex flex-col gap-4">
          <p className="text-[10px] font-mono text-slate-400 tracking-widest border-b border-white/8 pb-2">{title}</p>
          {items.map(({ label, sub, type, k, min, max, step }) => (
            <div key={k}>
              <div className="flex items-center justify-between mb-1.5">
                <div>
                  <p className="text-sm font-mono text-slate-200">{label}</p>
                  <p className="text-[10px] font-mono text-slate-500">{sub}</p>
                </div>
                {type === "toggle" && <Toggle k={k} />}
              </div>
              {type === "range" && (
                <input type="range" min={min} max={max} step={step} value={vals[k]}
                  onChange={e => slider(k, Number(e.target.value))}
                  className="w-full accent-sky-400" />
              )}
            </div>
          ))}
        </Card>
      ))}
    </div>
  );
}

// Root app
export default function App() {
  const [loggedIn, setLoggedIn] = useState(false);
  const [page, setPage] = useState("dashboard");
  const sensors = useSensors();
  const [tel, setTel] = useTelemetry();
  const [emergency, setEmergency] = useState(false);

  const handleStop = useCallback(() => {
    setEmergency(true);
    setTel(t => ({ ...t, speed: 0, gear: "P" }));
    setTimeout(() => setEmergency(false), 3000);
  }, [setTel]);

  if (!loggedIn) return <LoginPage onLogin={() => setLoggedIn(true)} />;

  const pages = {
    dashboard: <DashboardPage sensors={sensors} tel={tel} setPage={setPage} />,
    camera:    <CameraPage sensors={sensors} />,
    control:   <ControlPage tel={tel} setTel={setTel} />,
    sensors:   <SensorsPage sensors={sensors} />,
    autopark:  <AutoParkPage />,
    alerts:    <AlertsPage sensors={sensors} tel={tel} />,
    settings:  <SettingsPage />,
  };

  return (
    <div className="h-screen w-screen flex flex-col bg-[#030812] text-white overflow-hidden font-mono">
      <TopBar
        tel={tel} sensors={sensors}
        onStop={handleStop} emergencyActive={emergency}
        setPage={setPage} page={page}
      />
      <div className="flex flex-1 overflow-hidden">
        <Sidebar page={page} setPage={setPage} />
        <main className="flex-1 overflow-auto bg-[#040b18]">
          <AnimatePresence mode="wait">
            <motion.div key={page}
              initial={{ opacity: 0, y: 10 }}
              animate={{ opacity: 1, y: 0 }}
              exit={{ opacity: 0, y: -10 }}
              transition={{ duration: 0.18 }}
              className="h-full"
            >
              {pages[page]}
            </motion.div>
          </AnimatePresence>
        </main>
      </div>
      {/* Mobile E-Stop */}
      <div className="md:hidden flex items-center justify-center py-2 border-t border-white/8 bg-[#060d1a]">
        <EmergencyStop onStop={handleStop} active={emergency} />
      </div>
    </div>
  );
}
