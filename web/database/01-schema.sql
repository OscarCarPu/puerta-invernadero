-- Set timezone to Madrid (Europe/Madrid)
SET timezone = 'Europe/Madrid';

CREATE TABLE IF NOT EXISTS esp32_estado (
    id SERIAL PRIMARY KEY,
    ultima_conexion TIMESTAMPTZ NULL,
    esta_conectado BOOLEAN NOT NULL DEFAULT FALSE,
    temperatura DECIMAL(5, 2) NULL,
    humedad DECIMAL(5, 2) NULL,
    motor_apertura DECIMAL(5, 2) NULL,
    updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS lectura (
    id SERIAL PRIMARY KEY,
    temperatura DECIMAL(5, 2) NOT NULL,
    humedad DECIMAL(5, 2) NOT NULL,
    fecha_hora TIMESTAMPTZ NOT NULL DEFAULT NOW() UNIQUE
);

CREATE TABLE IF NOT EXISTS umbral (
    id SERIAL PRIMARY KEY,
    nombre VARCHAR(50) NOT NULL,
    tipo_umbral VARCHAR(20) CHECK (
        tipo_umbral IN ('temperatura', 'humedad')
    ) NOT NULL,
    valor_minimo DECIMAL(5, 2) NOT NULL,
    valor_maximo DECIMAL(5, 2) NOT NULL,
    activo BOOLEAN NOT NULL DEFAULT TRUE,
    updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE TRIGGER update_esp32_estado_updated_at
BEFORE UPDATE ON esp32_estado
FOR EACH ROW
EXECUTE FUNCTION UPDATE_UPDATED_AT_COLUMN();

CREATE TRIGGER update_umbrales_updated_at
BEFORE UPDATE ON umbral
FOR EACH ROW
EXECUTE FUNCTION UPDATE_UPDATED_AT_COLUMN();
