import { defineConfig } from "vitest/config";
import react from "@vitejs/plugin-react";

export default defineConfig({
  plugins: [react()],
  root: ".",
  base: "./",
  server: { port: 5173, strictPort: true },
  build: { outDir: "dist" },
  test: {
    environment: "node",
    include: ["tests/**/*.test.ts"],
  },
});
