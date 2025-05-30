-- Seed for umbral
INSERT INTO umbral (nombre, tipo_umbral, valor_minimo, valor_maximo, activo)
VALUES
('Umbral de Temperatura', 'temperatura', 20.00, 25.00, TRUE),
('Umbral de Humedad', 'humedad', 70.00, 85.00, TRUE);

-- Seed for esp32_estado
INSERT INTO esp32_estado (id)
VALUES (1);
