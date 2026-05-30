"use client";
import Link from "next/link";
import { usePathname } from "next/navigation";

const links = [
  { href: "/", label: "Home" },
  { href: "/upload", label: "Analyze" },
  { href: "/benchmark", label: "Benchmark" },
];

export default function Navbar() {
  const pathname = usePathname();
  return (
    <header
      style={{
        position: "sticky",
        top: 0,
        zIndex: 50,
        background: "rgba(255,255,255,0.92)",
        backdropFilter: "blur(12px)",
        borderBottom: "1px solid var(--border)",
        boxShadow: "0 1px 0 var(--border)",
      }}
    >
      <div
        style={{
          maxWidth: 1200,
          margin: "0 auto",
          padding: "0 24px",
          height: 56,
          display: "flex",
          alignItems: "center",
          justifyContent: "space-between",
        }}
      >
        {/* Logo */}
        <Link href="/" style={{ textDecoration: "none", display: "flex", alignItems: "center", gap: 10 }}>
          <div
            style={{
              width: 30,
              height: 30,
              borderRadius: 8,
              background: "var(--accent)",
              display: "flex",
              alignItems: "center",
              justifyContent: "center",
            }}
          >
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="white" strokeWidth="2.5" strokeLinecap="round" strokeLinejoin="round">
              <path d="M9 3H5a2 2 0 0 0-2 2v4m6-6h10a2 2 0 0 1 2 2v4M9 3v18m0 0h10a2 2 0 0 0 2-2v-4M9 21H5a2 2 0 0 1-2-2v-4m0 0h18" />
            </svg>
          </div>
          <span style={{ fontWeight: 700, fontSize: 16, color: "var(--text-primary)", letterSpacing: "-0.02em" }}>
            Pargus
          </span>
        </Link>

        {/* Nav links */}
        <nav style={{ display: "flex", alignItems: "center", gap: 4 }}>
          {links.map(({ href, label }) => {
            const active = pathname === href;
            return (
              <Link
                key={href}
                href={href}
                style={{
                  padding: "5px 14px",
                  borderRadius: 6,
                  fontSize: 13.5,
                  fontWeight: 500,
                  textDecoration: "none",
                  color: active ? "var(--accent)" : "var(--text-secondary)",
                  background: active ? "var(--accent-light)" : "transparent",
                  transition: "all 0.15s",
                }}
              >
                {label}
              </Link>
            );
          })}
        </nav>

        {/* Tag */}
        <div style={{ display: "flex", alignItems: "center", gap: 8 }}>
          <span className="badge badge-gray" style={{ fontSize: 11 }}>PDC Project · UET</span>
        </div>
      </div>
    </header>
  );
}
