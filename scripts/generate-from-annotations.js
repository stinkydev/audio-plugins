#!/usr/bin/env node

// Copyright 2025
// Generate TypeScript plugin definitions from C++ annotations

const fs = require('fs');
const path = require('path');

const PLUGINS = [
  { name: 'compressor', header: 'compressor/include/compressor_clap.h' },
  { name: 'delay', header: 'delay/include/delay_clap.h' },
  { name: 'eq', header: 'eq/include/eq_clap.h' },
  { name: 'limiter', header: 'limiter/include/limiter_clap.h' }
];

/**
 * Parse plugin metadata from @ts-plugin-meta comment block
 */
function parsePluginMeta(content) {
  const metaBlock = content.match(/@ts-plugin-meta\s*\n((?:\/\/[^\n]*\n)+)/);
  if (!metaBlock) {
    throw new Error('No @ts-plugin-meta block found');
  }
  
  const meta = {};
  const lines = metaBlock[1].split('\n');
  for (const line of lines) {
    const match = line.match(/\/\/\s*(\w+):\s*(.+)/);
    if (match) {
      meta[match[1]] = match[2].trim();
    }
  }
  
  return meta;
}

/**
 * Parse port annotations from @ts-port comments
 */
function parsePorts(content) {
  const ports = { inputs: [], outputs: [] };
  
  // Find all port annotations
  const portRegex = /\/\/\s*@ts-port\s+(.+)/g;
  let match;
  
  while ((match = portRegex.exec(content)) !== null) {
    const annotation = match[1];
    
    // Parse port attributes
    const attrs = {};
    const attrRegex = /(\w+)=("(?:[^"\\]|\\.)*"|[^\s]+)/g;
    let attrMatch;
    while ((attrMatch = attrRegex.exec(annotation)) !== null) {
      let value = attrMatch[2];
      // Remove quotes if present
      if (value.startsWith('"') && value.endsWith('"')) {
        value = value.slice(1, -1);
      }
      attrs[attrMatch[1]] = value;
    }
    
    // Add to appropriate port list
    const port = {
      id: parseInt(attrs.id || '0'),
      name: attrs.name || 'Port',
      channels: parseInt(attrs.channels || '2'),
      isMain: attrs.main === 'true'
    };
    
    if (attrs.type === 'input') {
      ports.inputs.push(port);
    } else if (attrs.type === 'output') {
      ports.outputs.push(port);
    }
  }
  
  // Sort ports by id
  ports.inputs.sort((a, b) => a.id - b.id);
  ports.outputs.sort((a, b) => a.id - b.id);
  
  return ports;
}

/**
 * Parse parameter annotations from enum
 */
function parseParameters(content) {
  const params = [];
  
  // Find all parameter lines with @ts-param annotations
  const paramRegex = /kParamId(\w+)(?:\s*=\s*(\d+))?\s*,?\s*\/\/\s*@ts-param\s+(.+)/g;
  let match;
  let currentId = 100;
  
  while ((match = paramRegex.exec(content)) !== null) {
    const [, name, explicitId, annotation] = match;
    
    if (explicitId) {
      currentId = parseInt(explicitId);
    }
    
    // Parse annotation attributes
    const attrs = {};
    // Match either quoted strings or non-space values
    const attrRegex = /(\w+)=("(?:[^"\\]|\\.)*"|[^\s]+)/g;
    let attrMatch;
    while ((attrMatch = attrRegex.exec(annotation)) !== null) {
      let value = attrMatch[2];
      // Remove quotes if present
      if (value.startsWith('"') && value.endsWith('"')) {
        value = value.slice(1, -1);
      }
      attrs[attrMatch[1]] = value;
    }
    
    const camelName = name.charAt(0).toLowerCase() + name.slice(1);
    
    params.push({
      name: camelName,
      originalName: name,
      id: currentId,
      attrs
    });
    
    currentId++;
  }
  
  return params;
}

/**
 * Generate TypeScript code from parsed data
 */
function generateTypeScript(pluginMeta, params, ports) {
  const className = pluginMeta.name;
  
  let code = `// Copyright 2025\n`;
  code += `// Auto-generated TypeScript definitions for ${className} plugin\n\n`;
  code += `import { IAudioPlugin } from "./audio-plugin";\n\n`;
  
  // Generate conversion functions
  const floatParams = params.filter(p => p.attrs.type !== 'bool' && p.attrs.type !== 'enum' && p.attrs.min && p.attrs.max);
  
  if (floatParams.length > 0) {
    code += `// Conversion functions from normalized [0,1] to actual values\n`;
    for (const param of floatParams) {
      const upperName = param.originalName;
      const funcName = `normalizedTo${upperName}`;
      const min = parseFloat(param.attrs.min);
      const max = parseFloat(param.attrs.max);
      
      if (param.attrs.scale === 'log') {
        code += `function ${funcName}(norm: number): number {\n`;
        code += `  return ${min} * Math.pow(${max} / ${min}, norm);\n`;
        code += `}\n\n`;
      } else {
        code += `function ${funcName}(norm: number): number {\n`;
        code += `  return ${min} + norm * (${max} - ${min});\n`;
        code += `}\n\n`;
      }
    }
    
    // Generate display text functions
    code += `// Display text functions with units\n`;
    for (const param of floatParams) {
      const funcName = `${param.name}ToText`;
      const convFunc = `normalizedTo${param.originalName}`;
      const unit = param.attrs.unit || '';
      
      if (unit === 'Hz') {
        code += `function ${funcName}(norm: number): string {\n`;
        code += `  const freq = ${convFunc}(norm);\n`;
        code += `  return freq >= 1000 ? \`\${(freq / 1000).toFixed(2)} kHz\` : \`\${freq.toFixed(1)} Hz\`;\n`;
        code += `}\n\n`;
      } else if (unit === '%') {
        code += `function ${funcName}(norm: number): string {\n`;
        code += `  return \`\${(${convFunc}(norm) * 100.0).toFixed(1)}%\`;\n`;
        code += `}\n\n`;
      } else {
        code += `function ${funcName}(norm: number): string {\n`;
        code += `  return \`\${${convFunc}(norm).toFixed(1)}${unit ? ' ' + unit : ''}\`;\n`;
        code += `}\n\n`;
      }
    }
  }
  
  // Generate plugin definition
  code += `export const ${className}Plugin: IAudioPlugin = {\n`;
  code += `  id: '${pluginMeta.id}',\n`;
  code += `  filename: '${pluginMeta.filename}',\n`;
  code += `  description: '${pluginMeta.description}',\n`;
  
  // Add ports if present
  if (ports && (ports.inputs.length > 0 || ports.outputs.length > 0)) {
    if (ports.inputs.length > 0) {
      code += `  inputPorts: [\n`;
      for (const port of ports.inputs) {
        code += `    { id: ${port.id}, name: '${port.name}', channels: ${port.channels}, isMain: ${port.isMain} },\n`;
      }
      code += `  ],\n`;
    }
    if (ports.outputs.length > 0) {
      code += `  outputPorts: [\n`;
      for (const port of ports.outputs) {
        code += `    { id: ${port.id}, name: '${port.name}', channels: ${port.channels}, isMain: ${port.isMain} },\n`;
      }
      code += `  ],\n`;
    }
  }
  
  code += `  params: [\n`;
  
  for (let i = 0; i < params.length; i++) {
    const param = params[i];
    const attrs = param.attrs;
    
    code += `    {\n`;
    code += `      name: '${param.name}',\n`;
    code += `      id: ${param.id},\n`;
    code += `      description: '${attrs.label || param.originalName}',\n`;
    code += `      label: '${attrs.label || param.originalName}',\n`;
    code += `      min: 0.0,\n`;
    code += `      max: 1.0,\n`;
    
    // Calculate normalized default value
    let defaultValue = '0.0';
    if (attrs.default !== undefined) {
      const def = parseFloat(attrs.default);
      if (attrs.min && attrs.max) {
        const min = parseFloat(attrs.min);
        const max = parseFloat(attrs.max);
        if (attrs.scale === 'log') {
          defaultValue = (Math.log(def / min) / Math.log(max / min)).toFixed(6);
        } else {
          defaultValue = ((def - min) / (max - min)).toFixed(6);
        }
      } else {
        defaultValue = def.toString();
      }
    }
    code += `      defaultValue: ${defaultValue},\n`;
    
    // Add display functions for float params
    if (attrs.type !== 'bool' && attrs.type !== 'enum' && attrs.min && attrs.max) {
      code += `      getDisplayValue: normalizedTo${param.originalName},\n`;
      code += `      getDisplayText: ${param.name}ToText,\n`;
    }
    
    // Add enum values if present
    if (attrs.type === 'enum' && attrs.values) {
      code += `      enumValues: [\n`;
      const values = attrs.values.split(',');
      values.forEach((val, idx) => {
        code += `        { value: ${idx}, label: '${val.trim()}' },\n`;
      });
      code += `      ],\n`;
    }
    
    code += `      type: '${attrs.type || 'float'}'\n`;
    code += `    }${i < params.length - 1 ? ',' : ''}\n`;
  }
  
  code += `  ]\n`;
  code += `};\n`;
  
  return code;
}

/**
 * Process a single plugin
 */
function processPlugin(plugin, rootDir) {
  console.log(`Processing ${plugin.name}...`);
  
  const headerPath = path.join(rootDir, plugin.header);
  
  if (!fs.existsSync(headerPath)) {
    console.warn(`  Warning: Header not found: ${headerPath}`);
    return;
  }
  
  const content = fs.readFileSync(headerPath, 'utf8');
  
  try {
    const meta = parsePluginMeta(content);
    const params = parseParameters(content);
    const ports = parsePorts(content);
    
    const tsCode = generateTypeScript(meta, params, ports);
    
    const outputPath = path.join(rootDir, 'ts', `${plugin.name}-plugin.ts`);
    fs.writeFileSync(outputPath, tsCode, 'utf8');
    
    console.log(`  ✓ Generated ${outputPath}`);
  } catch (error) {
    console.error(`  ✗ Error:`, error.message);
  }
}

/**
 * Main entry point
 */
function main() {
  const rootDir = path.resolve(__dirname, '..');
  
  console.log('Generating TypeScript plugin definitions from annotations...\n');
  
  for (const plugin of PLUGINS) {
    processPlugin(plugin, rootDir);
  }
  
  console.log('\nDone!');
}

if (require.main === module) {
  main();
}

module.exports = { parsePluginMeta, parseParameters, parsePorts, generateTypeScript };
