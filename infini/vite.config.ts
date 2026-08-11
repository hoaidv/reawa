import { defineConfig } from "vitest/config";
import react from "@vitejs/plugin-react";

export default defineConfig({
  plugins: [react()],
  root: ".",
  base: "./",
  server: { host: "127.0.0.1", port: 5173, strictPort: true },
  build: { outDir: "dist" },
  test: {
    environment: "node",
    include: ["tests/**/*.test.ts"],
  },
});
